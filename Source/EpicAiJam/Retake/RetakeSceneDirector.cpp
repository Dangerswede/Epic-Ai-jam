#include "Retake/RetakeSceneDirector.h"
#include "Retake/RetakeScenes.h"

#include "Components/BillboardComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/PostProcessVolume.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Retake/CameraMark.h"
#include "Retake/DirectorsCutComponent.h"
#include "Retake/RetakeCharacter.h"
#include "Retake/RetakeRecorderComponent.h"
#include "Retake/RetakeResettable.h"
#include "Retake/RetakeWorldReplayable.h"
#include "Retake/RetakeSaveGame.h"
#include "Retake/TakeGhost.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogRetakeDirector, Log, All);

ARetakeSceneDirector::ARetakeSceneDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	// Sample the world track after movement, like the take recorder.
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

#if WITH_EDITORONLY_DATA
	if (UBillboardComponent* Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite")))
	{
		Sprite->SetupAttachment(Root);
	}
#endif

	GhostClass = ATakeGhost::StaticClass();
	DirectorsCut = CreateDefaultSubobject<UDirectorsCutComponent>(TEXT("DirectorsCut"));
	ShotListText = FText::FromString(TEXT("Reach the mark."));
}

ARetakeSceneDirector* ARetakeSceneDirector::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ARetakeSceneDirector> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

void ARetakeSceneDirector::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<ACameraMark> It(GetWorld()); It; ++It)
	{
		Marks.Add(*It);
	}
	Marks.Sort([](const ACameraMark& A, const ACameraMark& B) { return A.MarkNumber < B.MarkNumber; });

	// F6: 70s film look for the whole scene (the Director's Cut camera adds its own sepia on top).
	if (bFilmLook)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		if (APostProcessVolume* Look = GetWorld()->SpawnActor<APostProcessVolume>(Params))
		{
			Look->bUnbound = true;
			Look->Priority = -10.f; // anything placed in the level wins
			FPostProcessSettings& PP = Look->Settings;
			PP.bOverride_FilmGrainIntensity = true;
			PP.FilmGrainIntensity = FilmGrain;
			PP.bOverride_SceneColorTint = true;
			PP.SceneColorTint = FilmTint;
			PP.bOverride_ColorSaturation = true;
			PP.ColorSaturation = FVector4(FilmSaturation, FilmSaturation, FilmSaturation, 1.f);
			PP.bOverride_ColorContrast = true;
			PP.ColorContrast = FVector4(1.06f, 1.06f, 1.06f, 1.f);
			PP.bOverride_ColorGain = true;
			PP.ColorGain = FVector4(1.03f, 1.f, 0.94f, 1.f);
			PP.bOverride_VignetteIntensity = true;
			PP.VignetteIntensity = FilmVignette;
		}
	}

	// ST1: the story table owns the scene titles (the Gap moved to scene 16).
	if (const RetakeScenes::FSceneInfo* Info = RetakeScenes::Find(GetSceneId()))
	{
		SceneTitle = FText::FromString(Info->Title);
	}
	if (RetakeScenes::IsFinale(GetSceneId()))
	{
		WrapSeconds = FMath::Max(WrapSeconds, 8.f); // S5: room for the two quiet lines before the card
	}

	// The player pawn may be spawned after level actors begin play; start next tick.
	GetWorldTimerManager().SetTimerForNextTick([this]()
	{
		if (!FindPlayer())
		{
			UE_LOG(LogRetakeDirector, Error, TEXT("No ARetakeCharacter found - is RetakeGameMode the game mode?"));
			return;
		}
		StartTransform = PlayerStart ? PlayerStart->GetActorTransform() : Player->GetActorTransform();
		SetState(ERetakeSceneState::Briefing);
		SetPlayerControl(false);
		if (bLocationScout && Player)
		{
			// F7: walk and look only - no E/R/Q, no jump (Space is the roll button here).
			if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
			{
				PC->SetIgnoreMoveInput(false);
			}
		}
		if (!bWaitForRollInput)
		{
			ScheduleState(BriefingSeconds, [this]() { StartRolling(); });
		}
	});
}

FString ARetakeSceneDirector::GetSceneId() const
{
	FString Name = GetWorld() ? GetWorld()->GetMapName() : FString();
	Name.RemoveFromStart(GetWorld() ? GetWorld()->StreamingLevelsPrefix : FString());
	return Name;
}

void ARetakeSceneDirector::HandlePlayerJumpPressed()
{
	HandleRollOrSkipInput();
}

void ARetakeSceneDirector::HandleRollOrSkipInput()
{
	if (State == ERetakeSceneState::Briefing || (State == ERetakeSceneState::Cut && bAwaitingRoll))
	{
		StartRolling();
	}
	else if (State == ERetakeSceneState::DirectorsCut)
	{
		FinishDirectorsCut();
	}
	else if (State == ERetakeSceneState::Results)
	{
		WatchDirectorsCut();
	}
}

void ARetakeSceneDirector::WatchDirectorsCut()
{
	if (State == ERetakeSceneState::Results && Takes.Num() > 0)
	{
		StartDirectorsCut();
	}
}

void ARetakeSceneDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	Super::EndPlay(EndPlayReason);
}

bool ARetakeSceneDirector::FindPlayer()
{
	if (Player)
	{
		return true;
	}
	if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		Player = Cast<ARetakeCharacter>(PC->GetPawn());
	}
	if (Player)
	{
		Player->OnCutRequested.AddUniqueDynamic(this, &ARetakeSceneDirector::HandlePlayerCut);
		Player->OnJumpPressed.AddUniqueDynamic(this, &ARetakeSceneDirector::HandlePlayerJumpPressed);
		Player->OnScrapRequested.AddUniqueDynamic(this, &ARetakeSceneDirector::HandlePlayerScrap);
		Player->OnRetakeRequested.AddUniqueDynamic(this, &ARetakeSceneDirector::HandlePlayerRetake);
		Player->OnAskDirector.AddUniqueDynamic(this, &ARetakeSceneDirector::AskDirector);
	}
	return Player != nullptr;
}

void ARetakeSceneDirector::SetState(ERetakeSceneState NewState)
{
	if (State == NewState)
	{
		return;
	}
	const ERetakeSceneState Old = State;
	State = NewState;
	UE_LOG(LogRetakeDirector, Log, TEXT("State: %s -> %s"),
		*UEnum::GetValueAsString(Old), *UEnum::GetValueAsString(NewState));
	OnStateChanged.Broadcast(NewState, Old);
}

void ARetakeSceneDirector::ScheduleState(float Delay, TFunction<void()> Fn)
{
	GetWorldTimerManager().ClearTimer(StateTimer);
	if (Delay <= 0.f)
	{
		GetWorldTimerManager().SetTimerForNextTick(MoveTemp(Fn));
		return;
	}
	GetWorldTimerManager().SetTimer(StateTimer, FTimerDelegate::CreateLambda(MoveTemp(Fn)), Delay, false);
}

void ARetakeSceneDirector::ResetResettables()
{
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (IRetakeResettable* Resettable = Cast<IRetakeResettable>(*It))
		{
			Resettable->ResetForTake();
		}
	}
}

void ARetakeSceneDirector::DestroyGhosts()
{
	for (ATakeGhost* Ghost : Ghosts)
	{
		if (IsValid(Ghost))
		{
			Ghost->Destroy();
		}
	}
	Ghosts.Reset();
}

void ARetakeSceneDirector::SpawnGhosts(const TArray<FTakeData>& TakesToPlay)
{
	DestroyGhosts();

	UClass* Class = GhostClass ? GhostClass.Get() : ATakeGhost::StaticClass();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (const FTakeData& Take : TakesToPlay)
	{
		if (Take.Frames.Num() == 0)
		{
			continue;
		}
		ATakeGhost* Ghost = GetWorld()->SpawnActor<ATakeGhost>(Class,
			Take.Frames[0].Location, Take.Frames[0].Rotation, Params);
		if (Ghost)
		{
			Ghost->Init(Take);
			const FText TakeRoleName = GetRoleName(Take.TakeNumber);
			if (!TakeRoleName.IsEmpty())
			{
				Ghost->SetLabelText(FString::Printf(TEXT("TAKE %d - %s"), RetakeScenes::DisplayTake(Take.TakeNumber), *TakeRoleName.ToString().ToUpper()));
			}
			Ghosts.Add(Ghost);
		}
	}
}

void ARetakeSceneDirector::RespawnPlayer()
{
	if (!Player)
	{
		return;
	}
	if (UCharacterMovementComponent* Move = Player->GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->SetMovementMode(MOVE_Walking);
	}
	Player->SetActorHiddenInGame(false);
	Player->SetActorEnableCollision(true);
	Player->SetActorLocationAndRotation(StartTransform.GetLocation(), StartTransform.GetRotation(),
		false, nullptr, ETeleportType::TeleportPhysics);
	if (AController* Controller = Player->GetController())
	{
		Controller->SetControlRotation(StartTransform.Rotator());
	}
}

void ARetakeSceneDirector::SetPlayerControl(bool bEnabled)
{
	if (!Player)
	{
		return;
	}
	Player->SetActionsEnabled(bEnabled);
	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		PC->ResetIgnoreMoveInput();
		PC->SetIgnoreMoveInput(!bEnabled);
	}
}

void ARetakeSceneDirector::StartRolling()
{
	if (!FindPlayer())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(StateTimer);
	GetWorldTimerManager().ClearTimer(DeathTimer);
	bDying = false;
	bAwaitingRoll = false;
	bResultsRecorded = false;
	UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
	CurrentTakeNumber = Takes.Num() + 1;

	ResetResettables();
	RespawnPlayer();
	SpawnGhosts(Takes);

	// Start recorder and ghosts on the same frame so they stay in sync.
	if (URetakeRecorderComponent* Recorder = Player->GetRecorder())
	{
		Recorder->StartRecording(CurrentTakeNumber);
	}
	SetWorldReplayMode(false);
	BeginWorldTrack();
	for (ATakeGhost* Ghost : Ghosts)
	{
		Ghost->Play(0.f);
	}

	TakeStartTime = GetWorld()->GetTimeSeconds();
	GetWorldTimerManager().SetTimer(TakeTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		CutTake(ERetakeCutReason::TimeUp);
	}), TakeDurationSeconds, false);

	SetPlayerControl(true);
	SetState(ERetakeSceneState::Rolling);
	OnTakeStarted.Broadcast(CurrentTakeNumber);
}

float ARetakeSceneDirector::GetTakeTimeRemaining() const
{
	if (State != ERetakeSceneState::Rolling)
	{
		return 0.f;
	}
	const float Elapsed = static_cast<float>(GetWorld()->GetTimeSeconds() - TakeStartTime);
	return FMath::Max(0.f, TakeDurationSeconds - Elapsed);
}

void ARetakeSceneDirector::HandlePlayerCut()
{
	CutTake(ERetakeCutReason::PlayerCut);
}

void ARetakeSceneDirector::CutTake(ERetakeCutReason Reason)
{
	if (State != ERetakeSceneState::Rolling || !Player)
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(TakeTimer);

	if (URetakeRecorderComponent* Recorder = Player->GetRecorder())
	{
		Takes.Add(Recorder->StopRecording());
		Takes.Last().EndReason = Reason;
		bAnyCaught |= Reason == ERetakeCutReason::Caught;
		bAnyDied |= Takes.Last().EndsInDeath();
	}
	bRecordingTrack = false;
	for (ATakeGhost* Ghost : Ghosts)
	{
		Ghost->Stop();
	}
	SetPlayerControl(false);

	++TotalTakesShot;
	UE_LOG(LogRetakeDirector, Log, TEXT("CUT! Take %d (%s)"), CurrentTakeNumber, *UEnum::GetValueAsString(Reason));
	SetState(ERetakeSceneState::Cut);
	OnTakeCut.Broadcast(CurrentTakeNumber, Reason);

	if (Takes.Num() >= MaxTakes)
	{
		// Out of film: don't wipe everything, just throw away this last take and roll it again.
		Takes.Pop();
		UE_LOG(LogRetakeDirector, Log, TEXT("Out of film: take %d scrapped."), CurrentTakeNumber);
		++ScrapCount;
		bRanOutOfFilm = true;
		OnOutOfFilm.Broadcast();
		RevealRoleTasks(); // G1: out of film - the director spells the jobs out
	}
	if (Roles.Num() > 0 && bWaitForRollInput)
	{
		// G2: with roles, the next take waits for Space (time to read the job), like the briefing.
		ScheduleState(CutPauseSeconds, [this]()
		{
			bAwaitingRoll = true;
			OnStateChanged.Broadcast(State, State); // refresh the HUD hint
		});
	}
	else
	{
		ScheduleState(CutPauseSeconds, [this]() { StartRolling(); });
	}
}

void ARetakeSceneDirector::HandlePlayerScrap()
{
	ScrapLastTake();
}

void ARetakeSceneDirector::HandlePlayerRetake()
{
	RetakeCurrentTake();
}

bool ARetakeSceneDirector::AbortRollingTake()
{
	if (State != ERetakeSceneState::Rolling || !Player || bDying)
	{
		return false;
	}
	GetWorldTimerManager().ClearTimer(TakeTimer);

	// Throw away what is being recorded right now (nothing is added to Takes).
	if (URetakeRecorderComponent* Recorder = Player->GetRecorder())
	{
		Recorder->StopRecording();
	}
	bRecordingTrack = false;
	for (ATakeGhost* Ghost : Ghosts)
	{
		Ghost->Stop();
	}
	SetPlayerControl(false);
	return true;
}

void ARetakeSceneDirector::RetakeCurrentTake()
{
	if (!AbortRollingTake())
	{
		return;
	}
	UE_LOG(LogRetakeDirector, Log, TEXT("Retake: take %d from the top."), CurrentTakeNumber);
	SetState(ERetakeSceneState::Cut);
	OnTakeRetaken.Broadcast(CurrentTakeNumber);
	ScheduleState(ScrapPauseSeconds, [this]() { StartRolling(); });
}

void ARetakeSceneDirector::ScrapLastTake()
{
	if (Takes.Num() == 0)
	{
		RetakeCurrentTake(); // K3: nothing to scrap yet (take 1) - Q just starts this take again
		return;
	}
	if (!AbortRollingTake())
	{
		return;
	}
	// Also throw away the last finished take, so that take rolls again.
	++ScrapCount;
	const int32 Scrapped = Takes.Last().TakeNumber;
	Takes.Pop();

	UE_LOG(LogRetakeDirector, Log, TEXT("Take %d scrapped."), Scrapped);
	SetState(ERetakeSceneState::Cut);
	OnTakeScrapped.Broadcast(Scrapped);
	ScheduleState(ScrapPauseSeconds, [this]() { StartRolling(); });
}

bool ARetakeSceneDirector::AreRoleTasksHidden() const
{
	// G1: reel 1 teaches with full job descriptions; from reel 2 on the job name is a hint and H asks for the rest.
	const RetakeScenes::FSceneInfo* Info = RetakeScenes::Find(GetSceneId());
	return Info && Info->Reel >= 2 && !bRoleTasksRevealed && Roles.Num() > 0;
}

void ARetakeSceneDirector::RevealRoleTasks()
{
	if (Roles.Num() == 0)
	{
		return;
	}
	bRoleTasksRevealed = true;
	OnRoleTasksRevealed.Broadcast();
	OnStateChanged.Broadcast(State, State); // refresh the HUD texts
}

void ARetakeSceneDirector::AskDirector()
{
	RevealRoleTasks();
}

FText ARetakeSceneDirector::GetRoleTask(int32 TakeNumber) const
{
	return Roles.IsValidIndex(TakeNumber - 1) ? Roles[TakeNumber - 1].Task : FText::GetEmpty();
}

FText ARetakeSceneDirector::GetRoleName(int32 TakeNumber) const
{
	if (bRolesAnyOrder)
	{
		return FText::GetEmpty(); // the player picks the job per take
	}
	return Roles.IsValidIndex(TakeNumber - 1) ? Roles[TakeNumber - 1].Name : FText::GetEmpty();
}

FText ARetakeSceneDirector::GetCastLine() const
{
	if (!bRolesAnyOrder || Roles.Num() == 0)
	{
		return FText::GetEmpty();
	}
	TArray<FString> Names;
	for (const FTakeRole& R : Roles)
	{
		Names.Add(R.Name.ToString().ToUpper());
	}
	return FText::FromString(TEXT("CAST: ") + FString::Join(Names, TEXT(" / ")));
}

FText ARetakeSceneDirector::GetRoleLine(int32 TakeNumber) const
{
	if (bRolesAnyOrder && Roles.Num() > 0)
	{
		// Order planning: list every job; the player decides which one this take does.
		TArray<FString> Jobs;
		for (const FTakeRole& R : Roles)
		{
			Jobs.Add(FString::Printf(TEXT("%s - %s"), *R.Name.ToString().ToUpper(), *R.Task.ToString()));
		}
		return FText::FromString(FString::Printf(TEXT("TAKE %d - YOUR CALL. Pick a job:\n%s"), RetakeScenes::DisplayTake(TakeNumber), *FString::Join(Jobs, TEXT("\n"))));
	}
	if (!Roles.IsValidIndex(TakeNumber - 1))
	{
		return FText::GetEmpty();
	}
	const FTakeRole& R = Roles[TakeNumber - 1];
	if (AreRoleTasksHidden())
	{
		return FText::FromString(FString::Printf(TEXT("TAKE %d - %s\n(H: ask the director what the job is)"), RetakeScenes::DisplayTake(TakeNumber),
			*R.Name.ToString().ToUpper()));
	}
	return FText::FromString(FString::Printf(TEXT("TAKE %d - %s: %s"), RetakeScenes::DisplayTake(TakeNumber),
		*R.Name.ToString().ToUpper(), *R.Task.ToString()));
}

bool ARetakeSceneDirector::IsBonusMet() const
{
	switch (BonusType)
	{
	case ERetakeBonus::FinalTakeUnderSeconds:
		return Takes.Num() > 0 && Takes.Last().EndReason == ERetakeCutReason::Wrapped && Takes.Last().Duration < BonusParam;
	case ERetakeBonus::NobodyCaught: return !bAnyCaught;
	case ERetakeBonus::SomeoneDied:  return bAnyDied;
	case ERetakeBonus::NoScraps:     return ScrapCount == 0;
	default:                         return true; // no note = nothing to miss
	}
}

FDirectorRating ARetakeSceneDirector::GetRating() const
{
	FDirectorRating R;
	const bool bWrapped = Takes.Num() > 0 && Takes.Last().EndReason == ERetakeCutReason::Wrapped;
	if (!bWrapped)
	{
		return R;
	}
	R.Wrap = FDirectorRating::WrapPoints;
	R.Par = Takes.Num() <= ParTakes ? FDirectorRating::ParPoints : 0;
	R.Note = IsBonusMet() ? FDirectorRating::NotePoints : 0;
	for (const ACameraMark* Mark : Marks)
	{
		const bool bHit = Mark && Takes.ContainsByPredicate([Mark](const FTakeData& T) { return Mark->IsHitBy(T); });
		R.MarksHit += bHit ? 1 : 0;
	}
	R.Marks = FMath::Min(FDirectorRating::MaxMarkPoints, R.MarksHit * FDirectorRating::PointsPerMark);
	return R;
}

int32 ARetakeSceneDirector::GetStars() const
{
	const bool bWrapped = Takes.Num() > 0 && Takes.Last().EndReason == ERetakeCutReason::Wrapped;
	return FDirectorRating::StarsFor(GetRating().Total(), bWrapped);
}

void ARetakeSceneDirector::NotifyMarkHit(ACameraMark* Mark)
{
	if (Mark && State == ERetakeSceneState::Rolling)
	{
		UE_LOG(LogRetakeDirector, Log, TEXT("Take %d hit mark %d"), CurrentTakeNumber, Mark->MarkNumber);
		OnMarkHit.Broadcast(Mark->MarkNumber);
	}
}

void ARetakeSceneDirector::NotifyPlayerKilled()
{
	if (State == ERetakeSceneState::Briefing && bLocationScout)
	{
		RespawnPlayer(); // K5: fell off the set while scouting - back to the start mark, nothing lost
		return;
	}
	if (bDying)
	{
		return; // already flying from a stunt death; let the tail finish
	}
	CutTake(ERetakeCutReason::Killed);
}

void ARetakeSceneDirector::BeginStuntDeath(ERetakeCutReason Reason, FVector Origin)
{
	if (State != ERetakeSceneState::Rolling || !Player || bDying)
	{
		return;
	}
	bDying = true;
	GetWorldTimerManager().ClearTimer(TakeTimer);
	SetPlayerControl(false);

	// Throw the stunt double away from whatever got them. The flight is recorded as ordinary frames.
	FVector Away = Player->GetActorLocation() - Origin;
	Away.Z = 0.f;
	Away = Away.IsNearlyZero() ? -Player->GetActorForwardVector() : Away.GetSafeNormal();
	Player->LaunchCharacter(Away * 900.f + FVector::UpVector * 700.f, true, true);
	Player->PlayStuntShake();

	// Brief slow-mo hit: 0.3x for 0.25 real seconds (timers run in dilated game time).
	if (StuntSlowMoDilation < 1.f)
	{
		UGameplayStatics::SetGlobalTimeDilation(this, StuntSlowMoDilation);
		FTimerHandle SlowMo;
		GetWorldTimerManager().SetTimer(SlowMo, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
		}), 0.25f * StuntSlowMoDilation, false);
	}

	UE_LOG(LogRetakeDirector, Log, TEXT("Stunt death (%s)"), *UEnum::GetValueAsString(Reason));
	GetWorldTimerManager().SetTimer(DeathTimer, FTimerDelegate::CreateWeakLambda(this, [this, Reason]()
	{
		if (Player && Player->GetRecorder())
		{
			Player->GetRecorder()->RecordAction(ERetakeAction::Died); // lands on the take's last frame
		}
		UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
		CutTake(Reason);
	}), FMath::Max(0.01f, StuntTailSeconds), false);
}

void ARetakeSceneDirector::NotifyGoalReached(AActor* Who)
{
	if (State != ERetakeSceneState::Rolling || !Player || Who != Player || bDying)
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(TakeTimer);

	// The winning take is part of the Director's Cut too.
	if (URetakeRecorderComponent* Recorder = Player->GetRecorder())
	{
		Takes.Add(Recorder->StopRecording());
		Takes.Last().EndReason = ERetakeCutReason::Wrapped;
	}
	// Keep the winning take's world track for the Director's Cut.
	SampleWorldTrack();
	bRecordingTrack = false;
	WinningTrack = CurrentTrack;
	for (ATakeGhost* Ghost : Ghosts)
	{
		Ghost->Stop();
	}
	SetPlayerControl(false);

	++TotalTakesShot;
	UE_LOG(LogRetakeDirector, Log, TEXT("That's a wrap! %d takes in the cut, %d shot."), Takes.Num(), TotalTakesShot);
	SetState(ERetakeSceneState::Wrap);
	OnWrap.Broadcast();
	// R1: straight to the results; the Director's Cut is optional (button / Space there).
	// G4: the Director's Cut is part of the show - it plays by itself at the big moments (Space skips).
	const bool bAutoCut = ShouldAutoPlayDirectorsCut();
	ScheduleState(WrapSeconds, [this, bAutoCut]() { if (bAutoCut) { StartDirectorsCut(); } else { EnterResults(); } });
}

void ARetakeSceneDirector::StartDirectorsCut()
{
	SetState(ERetakeSceneState::DirectorsCut);


	// Replay every take (including the winning one) together from time 0.
	ResetResettables();
	if (Player)
	{
		Player->SetActorHiddenInGame(true);
		Player->SetActorEnableCollision(false);
		Player->GetCharacterMovement()->DisableMovement();
	}
	SpawnGhosts(Takes);

	// Ghosts become "actors": opaque, one colour per take, the winning take in hero red.
	static const FLinearColor Palette[] = {
		FLinearColor(0.25f, 0.35f, 0.55f), FLinearColor(0.45f, 0.5f, 0.25f), FLinearColor(0.2f, 0.5f, 0.5f),
		FLinearColor(0.6f, 0.5f, 0.2f), FLinearColor(0.45f, 0.3f, 0.5f) };
	for (int32 i = 0; i < Ghosts.Num(); ++i)
	{
		const bool bWinner = (i == Ghosts.Num() - 1);
		if (RetakeScenes::IsFinale(GetSceneId()))
		{
			// S5: every take of Jack in warm gold, the last one brightest.
			Ghosts[i]->SetCinematicLook(bWinner ? FLinearColor(1.f, 0.62f, 0.25f) : FLinearColor(0.75f, 0.5f, 0.28f) * (0.8f + 0.1f * i));
			continue;
		}
		Ghosts[i]->SetCinematicLook(bWinner ? FLinearColor(0.85f, 0.08f, 0.06f) : Palette[i % UE_ARRAY_COUNT(Palette)]);
	}

	// World actors follow the winning take's recorded track instead of re-simulating.
	SetWorldReplayMode(true);
	ReplayTime = 0.f;
	bReplayingTrack = WinningTrack.Num() > 0;
	ApplyWorldTrack(0.f);

	float Longest = 0.f;
	for (ATakeGhost* Ghost : Ghosts)
	{
		Ghost->Play(0.f);
	}
	for (const FTakeData& Take : Takes)
	{
		Longest = FMath::Max(Longest, Take.Duration);
	}
	if (bCinematicDirectorsCut && DirectorsCut)
	{
		TArray<ATakeGhost*> Playing;
		for (ATakeGhost* Ghost : Ghosts)
		{
			Playing.Add(Ghost);
		}
		DirectorsCut->Begin(Playing, Longest); // calls FinishDirectorsCut() when done
	}
	else
	{
		ScheduleState(Longest + 1.f, [this]() { FinishDirectorsCut(); });
	}
}

void ARetakeSceneDirector::FinishDirectorsCut()
{
	if (State != ERetakeSceneState::DirectorsCut)
	{
		return;
	}
	bReplayingTrack = false;
	SetWorldReplayMode(false);
	if (DirectorsCut)
	{
		DirectorsCut->End();
	}
	EnterResults();
}

void ARetakeSceneDirector::EnterResults()
{
	// Save once per wrap (the results come back after watching the Director's Cut).
	if (!bResultsRecorded)
	{
		bResultsRecorded = true;
		bLastWrapNewBest = URetakeSaveGame::RecordWrap(GetSceneId(), Takes.Num(), GetStars());
	}
	GetWorldTimerManager().ClearTimer(StateTimer);
	for (ATakeGhost* Ghost : Ghosts)
	{
		Ghost->Stop();
	}
	SetState(ERetakeSceneState::Results);
}

bool ARetakeSceneDirector::CanSkipScene() const
{
	return bRanOutOfFilm || ScrapCount >= 3;
}

void ARetakeSceneDirector::SkipScene()
{
	if (!CanSkipScene())
	{
		return;
	}
	// G5: counts as done (0 stars) and unlocks the next scene, so nobody gets stuck on one puzzle.
	URetakeSaveGame::RecordSkip(GetSceneId());
	if (DirectorsCut && DirectorsCut->IsRunning())
	{
		DirectorsCut->End();
	}
	UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
	const int32 Index = RetakeScenes::IndexOf(GetSceneId());
	const bool bHasNext = RetakeScenes::All().IsValidIndex(Index + 1);
	UGameplayStatics::OpenLevel(this, FName(bHasNext ? RetakeScenes::All()[Index + 1].MapName : RetakeScenes::MainMenuMap()));
}

bool ARetakeSceneDirector::ShouldAutoPlayDirectorsCut() const
{
	const RetakeScenes::FSceneInfo* Info = RetakeScenes::Find(GetSceneId());
	if (!Info || !bCinematicDirectorsCut)
	{
		return false;
	}
	if (Info->bEndsReel || RetakeScenes::IsFinale(GetSceneId()))
	{
		return true; // end of every reel + the FOR JACK cut
	}
	// Scene 1, the first time it's wrapped: show what the Director's Cut is.
	const URetakeSaveGame* Save = URetakeSaveGame::LoadOrCreate();
	return Info->Number == 1 && !(Save && Save->IsWrapped(GetSceneId()));
}

void ARetakeSceneDirector::RestartScene()
{
	// K1: restarting from the pause menu during the Director's Cut must hand back the camera, overlay and time.
	if (DirectorsCut && DirectorsCut->IsRunning())
	{
		DirectorsCut->End();
	}
	UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
	GetWorldTimerManager().ClearTimer(TakeTimer);
	GetWorldTimerManager().ClearTimer(StateTimer);
	GetWorldTimerManager().ClearTimer(DeathTimer);
	if (Player && Player->GetRecorder() && Player->GetRecorder()->IsRecording())
	{
		Player->GetRecorder()->StopRecording();
	}
	TotalTakesShot = 0;
	ScrapCount = 0;
	bRanOutOfFilm = false;
	bAnyCaught = false;
	bAnyDied = false;
	bReplayingTrack = false;
	SetWorldReplayMode(false);
	Takes.Reset();
	DestroyGhosts();
	StartRolling();
}


// ------------------------------------------------------------------------------------ world track (P6)

void ARetakeSceneDirector::BeginWorldTrack()
{
	CurrentTrack.Reset();
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (Cast<IRetakeWorldReplayable>(*It))
		{
			CurrentTrack.Add(*It);
		}
	}
	TrackAccumulator = 0.f;
	bRecordingTrack = true;
	SampleWorldTrack(); // frame 0, same moment as the recorder's frame 0
}

void ARetakeSceneDirector::SampleWorldTrack()
{
	for (auto& Pair : CurrentTrack)
	{
		if (AActor* Actor = Pair.Key.Get())
		{
			FWorldTrackFrame& Frame = Pair.Value.AddDefaulted_GetRef();
			Frame.Transform = Actor->GetActorTransform();
			if (const IRetakeWorldReplayable* Replayable = Cast<IRetakeWorldReplayable>(Actor))
			{
				Frame.Extra = Replayable->GetReplayExtra();
			}
		}
	}
}

void ARetakeSceneDirector::ApplyWorldTrack(float Time)
{
	for (auto& Pair : WinningTrack)
	{
		AActor* Actor = Pair.Key.Get();
		const TArray<FWorldTrackFrame>& Frames = Pair.Value;
		if (!Actor || Frames.Num() == 0)
		{
			continue;
		}
		const float Index = Time * URetakeRecorderComponent::SampleRate;
		const int32 A = FMath::Clamp(FMath::FloorToInt(Index), 0, Frames.Num() - 1);
		const int32 B = FMath::Min(A + 1, Frames.Num() - 1);
		const float Alpha = (A == B) ? 0.f : FMath::Clamp(Index - A, 0.f, 1.f);
		FTransform Xf;
		Xf.Blend(Frames[A].Transform, Frames[B].Transform, Alpha);
		Actor->SetActorTransform(Xf, false, nullptr, ETeleportType::TeleportPhysics);
		if (IRetakeWorldReplayable* Replayable = Cast<IRetakeWorldReplayable>(Actor))
		{
			Replayable->ApplyReplayExtra(Frames[A].Extra);
		}
	}
}

void ARetakeSceneDirector::SetWorldReplayMode(bool bReplay)
{
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (IRetakeWorldReplayable* Replayable = Cast<IRetakeWorldReplayable>(*It))
		{
			Replayable->SetReplayMode(bReplay);
		}
	}
}

void ARetakeSceneDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bRecordingTrack && State == ERetakeSceneState::Rolling)
	{
		// Same fixed 30 Hz scheme as URetakeRecorderComponent.
		TrackAccumulator += DeltaSeconds;
		while (TrackAccumulator >= URetakeRecorderComponent::SampleInterval)
		{
			TrackAccumulator -= URetakeRecorderComponent::SampleInterval;
			SampleWorldTrack();
		}
	}
	if (bReplayingTrack && State == ERetakeSceneState::DirectorsCut)
	{
		ReplayTime += DeltaSeconds; // the same (dilated) game time the ghosts play in
		ApplyWorldTrack(ReplayTime);
	}
}
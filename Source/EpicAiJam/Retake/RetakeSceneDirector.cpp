#include "Retake/RetakeSceneDirector.h"

#include "Components/BillboardComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Retake/DirectorsCutComponent.h"
#include "Retake/RetakeCharacter.h"
#include "Retake/RetakeRecorderComponent.h"
#include "Retake/RetakeResettable.h"
#include "Retake/RetakeSaveGame.h"
#include "Retake/TakeGhost.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogRetakeDirector, Log, All);

ARetakeSceneDirector::ARetakeSceneDirector()
{
	PrimaryActorTick.bCanEverTick = false;

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
	if (State == ERetakeSceneState::Briefing)
	{
		StartRolling();
	}
	else if (State == ERetakeSceneState::DirectorsCut)
	{
		FinishDirectorsCut();
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
	CurrentTakeNumber = Takes.Num() + 1;

	ResetResettables();
	RespawnPlayer();
	SpawnGhosts(Takes);

	// Start recorder and ghosts on the same frame so they stay in sync.
	if (URetakeRecorderComponent* Recorder = Player->GetRecorder())
	{
		Recorder->StartRecording(CurrentTakeNumber);
	}
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
	}
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
		OnOutOfFilm.Broadcast();
	}
	ScheduleState(CutPauseSeconds, [this]() { StartRolling(); });
}

void ARetakeSceneDirector::HandlePlayerScrap()
{
	ScrapLastTake();
}

void ARetakeSceneDirector::ScrapLastTake()
{
	if (State != ERetakeSceneState::Rolling || !Player || Takes.Num() == 0)
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(TakeTimer);

	// Throw away the take being recorded right now AND the last finished take.
	if (URetakeRecorderComponent* Recorder = Player->GetRecorder())
	{
		Recorder->StopRecording();
	}
	const int32 Scrapped = Takes.Last().TakeNumber;
	Takes.Pop();
	for (ATakeGhost* Ghost : Ghosts)
	{
		Ghost->Stop();
	}
	SetPlayerControl(false);

	UE_LOG(LogRetakeDirector, Log, TEXT("Take %d scrapped."), Scrapped);
	SetState(ERetakeSceneState::Cut);
	OnTakeScrapped.Broadcast(Scrapped);
	ScheduleState(ScrapPauseSeconds, [this]() { StartRolling(); });
}

void ARetakeSceneDirector::NotifyPlayerKilled()
{
	CutTake(ERetakeCutReason::Killed);
}

void ARetakeSceneDirector::NotifyGoalReached(AActor* Who)
{
	if (State != ERetakeSceneState::Rolling || !Player || Who != Player)
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(TakeTimer);

	// The winning take is part of the Director's Cut too.
	if (URetakeRecorderComponent* Recorder = Player->GetRecorder())
	{
		Takes.Add(Recorder->StopRecording());
	}
	for (ATakeGhost* Ghost : Ghosts)
	{
		Ghost->Stop();
	}
	SetPlayerControl(false);

	++TotalTakesShot;
	UE_LOG(LogRetakeDirector, Log, TEXT("That's a wrap! %d takes in the cut, %d shot."), Takes.Num(), TotalTakesShot);
	SetState(ERetakeSceneState::Wrap);
	OnWrap.Broadcast();
	ScheduleState(WrapSeconds, [this]() { StartDirectorsCut(); });
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
	if (DirectorsCut)
	{
		DirectorsCut->End();
	}
	EnterResults();
}

void ARetakeSceneDirector::EnterResults()
{
	bLastWrapNewBest = URetakeSaveGame::RecordWrap(GetSceneId(), Takes.Num());
	GetWorldTimerManager().ClearTimer(StateTimer);
	for (ATakeGhost* Ghost : Ghosts)
	{
		Ghost->Stop();
	}
	SetState(ERetakeSceneState::Results);
}

void ARetakeSceneDirector::RestartScene()
{
	GetWorldTimerManager().ClearTimer(TakeTimer);
	GetWorldTimerManager().ClearTimer(StateTimer);
	if (Player && Player->GetRecorder() && Player->GetRecorder()->IsRecording())
	{
		Player->GetRecorder()->StopRecording();
	}
	TotalTakesShot = 0;
	Takes.Reset();
	DestroyGhosts();
	StartRolling();
}

#include "Retake/DirectorsCutComponent.h"
#include "Retake/RetakeScenes.h"

#include "CineCameraActor.h"
#include "CineCameraComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Retake/CameraMark.h"
#include "Retake/DirectorsCutOverlayWidget.h"
#include "Retake/ExplosiveProp.h"
#include "Retake/RetakeSceneDirector.h"
#include "Retake/TakeGhost.h"

UDirectorsCutComponent::UDirectorsCutComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bTickEvenWhenPaused = false;
	OverlayClass = UDirectorsCutOverlayWidget::StaticClass();
}

ARetakeSceneDirector* UDirectorsCutComponent::GetDirector() const
{
	return Cast<ARetakeSceneDirector>(GetOwner());
}

void UDirectorsCutComponent::ApplyLook(ACineCameraActor* Camera) const
{
	UCineCameraComponent* Comp = Camera ? Camera->GetCineCameraComponent() : nullptr;
	if (!Comp)
	{
		return;
	}
	Comp->FocusSettings.FocusMethod = ECameraFocusMethod::Disable;
	Comp->SetCurrentFocalLength(24.f);
	Comp->PostProcessBlendWeight = 1.f;
	FPostProcessSettings& PP = Comp->PostProcessSettings;
	PP.bOverride_FilmGrainIntensity = true;
	PP.FilmGrainIntensity = FilmGrain;
	PP.bOverride_ColorSaturation = true;
	PP.ColorSaturation = FVector4(Saturation, Saturation, Saturation, 1.f);
	PP.bOverride_SceneColorTint = true;
	PP.SceneColorTint = SepiaTint;
	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = Vignette;
}

void UDirectorsCutComponent::BuildMoments()
{
	TArray<FReelMoment> All;
	for (int32 i = 0; i < Ghosts.Num(); ++i)
	{
		const ATakeGhost* Ghost = Ghosts[i];
		if (!IsValid(Ghost))
		{
			continue;
		}
		const FTakeData& T = Ghost->GetTakeData();
		for (const FTakeFrame& F : T.Frames)
		{
			if (F.HasAction(ERetakeAction::Interact))
			{
				All.Add({ F.Time - ActionLead, F.Time + ActionTail, F.Time, FReelMoment::EKind::Action, i });
			}
		}
		if (const ARetakeSceneDirector* D = GetDirector())
		{
			for (int32 m = 0; m < D->GetMarks().Num(); ++m)
			{
				const ACameraMark* Mark = D->GetMarks()[m];
				const float HitTime = Mark ? Mark->FindHitTime(T) : -1.f;
				if (HitTime >= 0.f)
				{
					FReelMoment Moment{ HitTime - 0.8f, HitTime + 1.4f, HitTime, FReelMoment::EKind::Mark, i };
					Moment.MarkIndex = m;
					All.Add(Moment);
				}
			}
		}
		if (T.EndsInDeath())
		{
			// The launch happened ~0.6 s before the last frame (the recorded stunt tail).
			All.Add({ T.Duration - 1.8f, T.Duration + 0.2f, T.Duration - 0.6f, FReelMoment::EKind::Death, i });
		}
		if (i == Ghosts.Num() - 1 && T.EndReason == ERetakeCutReason::Wrapped)
		{
			All.Add({ T.Duration - GoalLead, T.Duration + 0.3f, T.Duration, FReelMoment::EKind::Goal, i });
		}
	}
	for (FReelMoment& M : All)
	{
		M.Start = FMath::Max(0.f, M.Start);
	}

	// Budget: always keep the goal and the stunt deaths, then as many actions as fit (earliest first).
	auto Cost = [this](const FReelMoment& M)
	{
		const float Len = M.End - M.Start;
		const bool bSlow = M.Kind == FReelMoment::EKind::Death || M.Kind == FReelMoment::EKind::Goal;
		return bSlow ? Len + 1.f / SlowMoDilation - 1.f : Len;
	};
	All.Sort([](const FReelMoment& A, const FReelMoment& B)
	{
		if (A.Kind != B.Kind) return A.Kind > B.Kind; // Goal > Death > Action
		return A.Start < B.Start;
	});
	float Budget = MaxReelSeconds - TitleCardSeconds;
	TArray<FReelMoment> Kept;
	for (const FReelMoment& M : All)
	{
		if (M.Kind != FReelMoment::EKind::Action || Cost(M) <= Budget)
		{
			Kept.Add(M);
			Budget -= Cost(M);
		}
	}

	// Timeline order, overlapping windows merged (the higher-priority subject wins).
	Kept.Sort([](const FReelMoment& A, const FReelMoment& B) { return A.Start < B.Start; });
	Moments.Reset();
	for (const FReelMoment& M : Kept)
	{
		if (Moments.Num() > 0 && M.Start <= Moments.Last().End + 0.3f)
		{
			FReelMoment& Prev = Moments.Last();
			Prev.End = FMath::Max(Prev.End, M.End);
			if (M.Kind > Prev.Kind)
			{
				Prev.Kind = M.Kind;
				Prev.Focus = M.Focus;
				Prev.GhostIndex = M.GhostIndex;
			}
			if (Prev.MarkIndex == INDEX_NONE)
			{
				Prev.MarkIndex = M.MarkIndex; // still shoot it from the mark camera if one was hit
			}
			continue;
		}
		Moments.Add(M);
	}
}

void UDirectorsCutComponent::Begin(const TArray<ATakeGhost*>& InGhosts, float LongestTakeSeconds)
{
	Ghosts.Reset();
	for (ATakeGhost* Ghost : InGhosts)
	{
		Ghosts.Add(Ghost);
	}
	ReplayLength = LongestTakeSeconds;
	Elapsed = 0.f;
	RealElapsed = 0.f;
	EndCardAtReal = 0.f;
	ExplosionRealTime = -100.f;
	CurrentDilation = 1.f;
	CurrentMoment = INDEX_NONE;
	LastShotOffset = FVector(NAN);
	LastMarkCam.Reset();
	MarkShotsUsed = 0;
	bEndCardShown = false;
	bRunning = true;
	BuildMoments();

	for (TActorIterator<AExplosiveProp> It(GetWorld()); It; ++It)
	{
		It->OnExploded.AddUniqueDynamic(this, &UDirectorsCutComponent::HandleExploded);
	}

	if (!ReelCamera)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ReelCamera = GetWorld()->SpawnActor<ACineCameraActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	}
	ApplyLook(ReelCamera);

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (!Overlay)
		{
			Overlay = CreateWidget<UDirectorsCutOverlayWidget>(PC, OverlayClass ? OverlayClass.Get() : UDirectorsCutOverlayWidget::StaticClass());
		}
		if (Overlay && !Overlay->IsInViewport())
		{
			Overlay->AddToViewport(5);
		}
		if (Overlay)
		{
			const ARetakeSceneDirector* D = GetDirector();
			Overlay->ShowTitle(TEXT("DIRECTOR'S CUT"), D ? D->SceneTitle.ToString() : FString(), TitleCardSeconds);
			Overlay->ShowBlack(true);
		}
		PC->SetViewTarget(ReelCamera);
	}
	SetComponentTickEnabled(true);
}

void UDirectorsCutComponent::End()
{
	if (!bRunning)
	{
		return;
	}
	bRunning = false;
	SetComponentTickEnabled(false);
	UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
	CurrentDilation = 1.f;
	for (TActorIterator<AExplosiveProp> It(GetWorld()); It; ++It)
	{
		It->OnExploded.RemoveDynamic(this, &UDirectorsCutComponent::HandleExploded);
	}
	if (Overlay)
	{
		Overlay->ShowBlack(false);
		Overlay->RemoveFromParent();
	}
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			PC->SetViewTarget(Pawn);
		}
	}
	Ghosts.Reset();
}

const FReelMoment* UDirectorsCutComponent::MomentAt(float Time) const
{
	for (const FReelMoment& M : Moments)
	{
		if (Time >= M.Start && Time <= M.End)
		{
			return &M;
		}
	}
	return nullptr;
}

void UDirectorsCutComponent::HandleExploded(AExplosiveProp* Prop)
{
	if (bRunning && Prop)
	{
		ExplosionAt = Prop->GetActorLocation();
		ExplosionRealTime = RealElapsed;
	}
}

FVector UDirectorsCutComponent::ResolveCameraLocation(const FVector& Desired, const FVector& Target) const
{
	// True line of sight from the camera spot to the subject (ghosts and the hidden player don't count).
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ReelCamera), false, GetOwner());
	for (const ATakeGhost* G : Ghosts)
	{
		Params.AddIgnoredActor(G);
	}
	if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		Params.AddIgnoredActor(PC->GetPawn());
	}
	FHitResult Hit;
	return GetWorld()->LineTraceSingleByChannel(Hit, Target, Desired, ECC_Visibility, Params) ? FVector(NAN) : Desired;
}

const ACameraMark* UDirectorsCutComponent::PickMarkCamera(const FReelMoment& Moment, const FVector& LookAt) const
{
	const ARetakeSceneDirector* D = GetDirector();
	if (!D)
	{
		return nullptr;
	}
	auto Clear = [&](const ACameraMark* Mark)
	{
		return Mark && FVector::Dist(Mark->GetCameraWorldLocation(), LookAt) < 2600.f
			&& !ResolveCameraLocation(Mark->GetCameraWorldLocation(), LookAt).ContainsNaN();
	};
	// 1) The mark this moment is about. 2) Stay on the current mark camera while it sees the subject.
	// 3) The nearest mark camera with a clear view. Otherwise the dynamic camera is the fallback.
	if (D->GetMarks().IsValidIndex(Moment.MarkIndex) && Clear(D->GetMarks()[Moment.MarkIndex]))
	{
		return D->GetMarks()[Moment.MarkIndex];
	}
	if (Clear(LastMarkCam.Get()))
	{
		return LastMarkCam.Get();
	}
	const ACameraMark* Best = nullptr;
	float BestDist = TNumericLimits<float>::Max();
	for (const ACameraMark* Mark : D->GetMarks())
	{
		const float Dist = Mark ? FVector::Dist(Mark->GetCameraWorldLocation(), LookAt) : BestDist;
		if (Dist < BestDist && Clear(Mark))
		{
			Best = Mark;
			BestDist = Dist;
		}
	}
	return Best;
}

void UDirectorsCutComponent::UpdateCamera(const FReelMoment* Moment, float RealDelta, bool bSnap)
{
	if (!ReelCamera || !Moment || !Ghosts.IsValidIndex(Moment->GhostIndex) || !IsValid(Ghosts[Moment->GhostIndex]))
	{
		return;
	}
	const ATakeGhost* Subject = Ghosts[Moment->GhostIndex];

	// F1: cut between the fixed camera-mark cameras; they pan to follow the subject.
	const FVector Aim = Subject->GetActorLocation() + FVector(0.f, 0.f, 60.f);
	if (const ACameraMark* MarkCam = PickMarkCamera(*Moment, Aim))
	{
		const bool bNewShot = MarkCam != LastMarkCam.Get();
		if (bNewShot)
		{
			++MarkShotsUsed;
			if (Overlay)
			{
				Overlay->ShowShotLabel(CurrentShotLabel + FString::Printf(TEXT("   |   MARK %d%s%s"), MarkCam->MarkNumber,
					MarkCam->ShotName.IsEmpty() ? TEXT("") : TEXT(": "), *MarkCam->ShotName.ToUpper()));
			}
		}
		LastMarkCam = MarkCam;
		LastShotOffset = FVector(NAN);
		const FVector CamLoc = MarkCam->GetCameraWorldLocation();
		const FRotator WantRot = (Aim - CamLoc).Rotation();
		const bool bHard = bSnap || bNewShot;
		ReelCamera->SetActorLocationAndRotation(CamLoc, bHard ? WantRot : FMath::RInterpTo(ReelCamera->GetActorRotation(), WantRot, RealDelta, 8.f));
		return;
	}
	if (LastMarkCam.IsValid())
	{
		LastMarkCam.Reset();
		bSnap = true;
		if (Overlay)
		{
			Overlay->ShowShotLabel(CurrentShotLabel);
		}
	}
	// Explosions slow the reel down (see Tick) but the camera stays on the actor: the blast throws them.
	const bool bExplosionShot = false;
	const bool bFinalWide = Moment->Kind == FReelMoment::EKind::Goal && Elapsed > Moment->End - 1.4f;
	const FVector Loc = bExplosionShot ? ExplosionAt : Subject->GetActorLocation();
	const FVector LookAt = Loc + FVector(0.f, 0.f, bExplosionShot ? 90.f : 60.f);
	FVector Fwd = Subject->GetActorForwardVector().GetSafeNormal2D();
	if (bExplosionShot)
	{
		Fwd = (ExplosionAt - Subject->GetActorLocation()).GetSafeNormal2D();
		Fwd = Fwd.IsNearlyZero() ? FVector::ForwardVector : Fwd;
	}
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Fwd);
	const FVector Up = FVector::UpVector;

	// Candidate shots in order of preference; the first with a clear view of the subject wins.
	// Narrow corridors block the side/low shots, so an overhead shot is always the last resort.
	TArray<FVector, TInlineAllocator<6>> Offsets;
	if (bFinalWide)
	{
		Offsets = { -Fwd * 650.f + Side * 450.f + Up * 420.f, -Fwd * 650.f - Side * 450.f + Up * 420.f, -Fwd * 500.f + Up * 600.f };
	}
	else if (bExplosionShot || Moment->Kind == FReelMoment::EKind::Death)
	{
		Offsets = { Side * 360.f - Fwd * 120.f + Up * 50.f, -Side * 360.f - Fwd * 120.f + Up * 50.f, -Fwd * 420.f + Up * 120.f, -Fwd * 300.f + Up * 450.f };
	}
	else
	{
		Offsets = { -Fwd * 380.f + Up * 200.f, -Fwd * 300.f + Side * 250.f + Up * 220.f, -Fwd * 300.f - Side * 250.f + Up * 220.f, -Fwd * 250.f + Up * 500.f };
	}
	FVector Desired = FVector(NAN);
	if (!bSnap && !LastShotOffset.ContainsNaN())
	{
		Desired = ResolveCameraLocation(Loc + LastShotOffset, LookAt); // keep the chosen angle while it stays clear
	}
	if (Desired.ContainsNaN())
	{
		for (const FVector& O : Offsets)
		{
			Desired = ResolveCameraLocation(Loc + O, LookAt);
			if (!Desired.ContainsNaN())
			{
				LastShotOffset = O;
				bSnap = true; // new angle = hard cut
				break;
			}
		}
	}
	if (Desired.ContainsNaN())
	{
		LastShotOffset = -Fwd * 150.f + Up * 900.f; // straight overhead: open sky above every set
		Desired = Loc + LastShotOffset;
		bSnap = true;
	}

	const FVector NewLoc = bSnap ? Desired : FMath::VInterpTo(ReelCamera->GetActorLocation(), Desired, RealDelta, 6.f);
	const FRotator WantRot = (LookAt - NewLoc).Rotation();
	const FRotator NewRot = bSnap ? WantRot : FMath::RInterpTo(ReelCamera->GetActorRotation(), WantRot, RealDelta, 10.f);
	ReelCamera->SetActorLocationAndRotation(NewLoc, NewRot);
}
void UDirectorsCutComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bRunning)
	{
		return;
	}
	// DeltaTime is dilated game time; the reel length and title cards run in real time.
	const float RealDelta = DeltaTime / FMath::Max(0.01f, CurrentDilation);
	RealElapsed += RealDelta;
	Elapsed += DeltaTime;

	const float LastMomentEnd = Moments.Num() > 0 ? Moments.Last().End : 0.f;
	if (!bEndCardShown && (Elapsed >= FMath::Min(ReplayLength, LastMomentEnd) || Moments.Num() == 0))
	{
		bEndCardShown = true;
		EndCardAtReal = RealElapsed;
		CurrentDilation = 1.f;
		UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
		if (Overlay)
		{
			Overlay->ShowBlack(false);
			Overlay->ShowTitle(TEXT("THAT'S A WRAP"), FString::Printf(TEXT("%d takes"), Ghosts.Num()), TitleCardSeconds);
		}
	}
	if (bEndCardShown)
	{
		if (RealElapsed >= EndCardAtReal + TitleCardSeconds)
		{
			if (ARetakeSceneDirector* D = GetDirector())
			{
				D->FinishDirectorsCut(); // calls End()
			}
			else
			{
				End();
			}
		}
		return;
	}

	// Inside a moment: play it (slow-mo at its peak); between moments: skip fast behind a black frame.
	const FReelMoment* M = MomentAt(Elapsed);
	const int32 Index = M ? static_cast<int32>(M - Moments.GetData()) : INDEX_NONE;
	const bool bCut = Index != CurrentMoment;
	CurrentMoment = Index;

	float Target = SkipDilation;
	if (M)
	{
		const bool bPeak = M->Kind != FReelMoment::EKind::Action && FMath::Abs(Elapsed - M->Focus) < 0.5f;
		const bool bBlast = RealElapsed - ExplosionRealTime < 1.f;
		Target = (bPeak || bBlast) ? SlowMoDilation : 1.f;
		if (bCut && Overlay)
		{
			const ATakeGhost* G = Ghosts.IsValidIndex(M->GhostIndex) ? Ghosts[M->GhostIndex].Get() : nullptr;
			const ARetakeSceneDirector* D = GetDirector();
			const FText Role = (G && D) ? D->GetRoleName(G->GetTakeNumber()) : FText::GetEmpty();
			CurrentShotLabel = !G ? FString(TEXT("ALL TAKES")) : Role.IsEmpty()
				? FString::Printf(TEXT("TAKE %d"), RetakeScenes::DisplayTake(G->GetTakeNumber()))
				: FString::Printf(TEXT("TAKE %d - %s"), RetakeScenes::DisplayTake(G->GetTakeNumber()), *Role.ToString().ToUpper());
			Overlay->ShowShotLabel(CurrentShotLabel);
			LastMarkCam.Reset(); // new moment = new shot
		}
		UpdateCamera(M, RealDelta, bCut);
	}
	if (Overlay)
	{
		Overlay->ShowBlack(M == nullptr);
	}
	if (!FMath::IsNearlyEqual(Target, CurrentDilation))
	{
		CurrentDilation = Target;
		UGameplayStatics::SetGlobalTimeDilation(this, CurrentDilation);
	}
}

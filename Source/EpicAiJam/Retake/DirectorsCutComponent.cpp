#include "Retake/DirectorsCutComponent.h"

#include "CineCameraActor.h"
#include "CineCameraComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Retake/DirectorsCutOverlayWidget.h"
#include "Retake/ExplosiveProp.h"
#include "Retake/RetakeSceneDirector.h"
#include "Retake/TakeGhost.h"

UDirectorsCutComponent::UDirectorsCutComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	OverlayClass = UDirectorsCutOverlayWidget::StaticClass();
}

ARetakeSceneDirector* UDirectorsCutComponent::GetDirector() const
{
	return Cast<ARetakeSceneDirector>(GetOwner());
}

void UDirectorsCutComponent::GatherCameras()
{
	Cameras.Reset();
	WideCamera = nullptr;
	for (TActorIterator<ACineCameraActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(CameraTag))
		{
			Cameras.Add(*It);
			if (It->ActorHasTag(WideCameraTag) && !WideCamera)
			{
				WideCamera = *It;
			}
		}
	}

	if (Cameras.Num() == 0)
	{
		// Fallback: one wide camera looking at the middle of all ghosts.
		FVector Center = FVector::ZeroVector;
		for (const ATakeGhost* Ghost : Ghosts)
		{
			Center += Ghost->GetActorLocation();
		}
		Center /= FMath::Max(1, Ghosts.Num());
		const FVector Pos = Center + FVector(-1400.f, -1400.f, 1100.f);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnedFallback = GetWorld()->SpawnActor<ACineCameraActor>(Pos, (Center - Pos).Rotation(), Params);
		if (SpawnedFallback)
		{
			SpawnedFallback->GetCineCameraComponent()->SetCurrentFocalLength(18.f);
			Cameras.Add(SpawnedFallback);
			WideCamera = SpawnedFallback;
		}
	}
	if (!WideCamera && Cameras.Num() > 0)
	{
		WideCamera = Cameras[0];
	}
}

void UDirectorsCutComponent::ApplyLook(ACineCameraActor* Camera) const
{
	UCineCameraComponent* Comp = Camera ? Camera->GetCineCameraComponent() : nullptr;
	if (!Comp)
	{
		return;
	}
	Comp->FocusSettings.FocusMethod = ECameraFocusMethod::Disable;
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

void UDirectorsCutComponent::Begin(const TArray<ATakeGhost*>& InGhosts, float LongestTakeSeconds)
{
	Ghosts.Reset();
	for (ATakeGhost* Ghost : InGhosts)
	{
		Ghosts.Add(Ghost);
	}
	ReplayLength = LongestTakeSeconds;
	Elapsed = 0.f;
	NextCutAt = 0.f;
	bEndCardShown = false;
	bRunning = true;
	Rng.Initialize(1337);
	RealElapsed = 0.f;
	EndCardAtReal = 0.f;
	SlowMoUntilReal = -1.f;
	CurrentDilation = 1.f;
	for (TActorIterator<AExplosiveProp> It(GetWorld()); It; ++It)
	{
		It->OnExploded.AddUniqueDynamic(this, &UDirectorsCutComponent::HandleExploded);
	}

	GatherCameras();
	for (ACineCameraActor* Cam : Cameras)
	{
		ApplyLook(Cam);
	}

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
		}
	}

	// Open on the wide shot.
	CutTo(WideCamera);
	NextCutAt = Rng.FRandRange(MinShotSeconds, MaxShotSeconds); // real seconds
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
		Overlay->RemoveFromParent();
	}
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			PC->SetViewTarget(Pawn);
		}
	}
	if (SpawnedFallback)
	{
		SpawnedFallback->Destroy();
		SpawnedFallback = nullptr;
	}
	CurrentCamera = nullptr;
	Ghosts.Reset();
}

ATakeGhost* UDirectorsCutComponent::PickInterestingGhost() const
{
	ATakeGhost* Best = nullptr;
	float BestRecency = TNumericLimits<float>::Max();
	for (ATakeGhost* Ghost : Ghosts)
	{
		if (!IsValid(Ghost) || Ghost->GetLastActionTime() < 0.f)
		{
			continue;
		}
		const float Since = Ghost->GetPlaybackTime() - Ghost->GetLastActionTime();
		if (Since >= 0.f && Since < ActionInterestSeconds && Since < BestRecency)
		{
			BestRecency = Since;
			Best = Ghost;
		}
	}
	if (Best)
	{
		return Best;
	}
	// Otherwise the fastest-moving ghost that is still playing.
	float BestSpeed = -1.f;
	for (ATakeGhost* Ghost : Ghosts)
	{
		if (IsValid(Ghost) && Ghost->IsPlaying() && Ghost->GetVelocity().Size() > BestSpeed)
		{
			BestSpeed = Ghost->GetVelocity().Size();
			Best = Ghost;
		}
	}
	return Best;
}

void UDirectorsCutComponent::CutTo(ACineCameraActor* Camera)
{
	if (!Camera)
	{
		return;
	}
	CurrentCamera = Camera;
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->SetViewTarget(Camera); // hard cut
	}
	if (Overlay)
	{
		const ATakeGhost* S = Subject.Get();
		Overlay->ShowShotLabel(S ? FString::Printf(TEXT("TAKE %d"), S->GetTakeNumber()) : FString(TEXT("ALL TAKES")));
	}
}

ACineCameraActor* UDirectorsCutComponent::NearestCamera(const FVector& Location, bool bAvoidCurrent) const
{
	ACineCameraActor* Best = nullptr;
	float BestDist = TNumericLimits<float>::Max();
	for (ACineCameraActor* Cam : Cameras)
	{
		if (bAvoidCurrent && Cam == CurrentCamera && Cameras.Num() > 1)
		{
			continue;
		}
		const float D = FVector::DistSquared(Cam->GetActorLocation(), Location);
		if (D < BestDist)
		{
			BestDist = D;
			Best = Cam;
		}
	}
	return Best ? Best : WideCamera.Get();
}

void UDirectorsCutComponent::ChooseShot()
{
	ATakeGhost* Target = PickInterestingGhost();
	Subject = Target;
	if (!Target || Cameras.Num() <= 1)
	{
		CutTo(WideCamera);
		return;
	}
	// Nearest camera to the subject, avoiding the current one so every cut is visible.
	CutTo(NearestCamera(Target->GetActorLocation(), true));
}

void UDirectorsCutComponent::HandleExploded(AExplosiveProp* Prop)
{
	if (!bRunning || !Prop)
	{
		return;
	}
	SlowMoUntilReal = RealElapsed + 1.2f;
	// Always cut to the camera nearest the blast.
	Subject = nullptr;
	CutTo(NearestCamera(Prop->GetActorLocation(), false));
	NextCutAt = RealElapsed + MaxShotSeconds;
}

float UDirectorsCutComponent::ComputeTargetDilation() const
{
	if (RealElapsed < SlowMoUntilReal)
	{
		return SlowMoDilation; // explosion: slow-mo
	}
	const ATakeGhost* Winner = Ghosts.Num() > 0 ? Ghosts.Last().Get() : nullptr;
	if (IsValid(Winner) && Winner->IsPlaying() && ReplayLength - Winner->GetPlaybackTime() < 1.f)
	{
		return SlowMoDilation; // the hero's final second
	}
	bool bAnyMoving = false;
	float NextAction = TNumericLimits<float>::Max();
	for (const ATakeGhost* Ghost : Ghosts)
	{
		if (!IsValid(Ghost) || !Ghost->IsPlaying())
		{
			continue;
		}
		bAnyMoving |= Ghost->GetVelocity().Size() > 20.f;
		NextAction = FMath::Min(NextAction, Ghost->GetTimeToNextAction());
	}
	if (NextAction > 1.5f)
	{
		// Nothing about to happen: fast-forward if everyone stands still, otherwise a brisk "cruise".
		return bAnyMoving ? CruiseDilation : FastForwardDilation;
	}
	return 1.f;
}

void UDirectorsCutComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bRunning)
	{
		return;
	}
	// DeltaTime is dilated game time; cut timing and title cards run in real time.
	const float RealDelta = DeltaTime / FMath::Max(0.01f, CurrentDilation);
	RealElapsed += RealDelta;
	Elapsed += DeltaTime;

	// Pacing: slow-mo for big moments, fast-forward through dead time.
	const float Target = bEndCardShown ? 1.f : ComputeTargetDilation();
	CurrentDilation = FMath::FInterpTo(CurrentDilation, Target, RealDelta, 6.f);
	UGameplayStatics::SetGlobalTimeDilation(this, CurrentDilation);

	if (!bEndCardShown && RealElapsed >= NextCutAt)
	{
		ChooseShot();
		NextCutAt = RealElapsed + Rng.FRandRange(MinShotSeconds, MaxShotSeconds);
	}

	// Gently keep the subject framed (the wide shot stays locked).
	ATakeGhost* Subj = Subject.Get();
	if (CurrentCamera && CurrentCamera != WideCamera && IsValid(Subj))
	{
		const FRotator Want = (Subj->GetActorLocation() - CurrentCamera->GetActorLocation()).Rotation();
		CurrentCamera->SetActorRotation(FMath::RInterpTo(CurrentCamera->GetActorRotation(), Want, RealDelta, 3.f));
	}

	if (!bEndCardShown && Elapsed >= ReplayLength)
	{
		bEndCardShown = true;
		EndCardAtReal = RealElapsed;
		// End on the hero.
		ATakeGhost* Winner = Ghosts.Num() > 0 ? Ghosts.Last().Get() : nullptr;
		Subject = Winner;
		CutTo(Winner ? NearestCamera(Winner->GetActorLocation(), false) : WideCamera.Get());
		if (Overlay)
		{
			Overlay->ShowTitle(TEXT("THAT'S A WRAP"), FString::Printf(TEXT("%d takes"), Ghosts.Num()), TitleCardSeconds);
		}
	}
	if (bEndCardShown && RealElapsed >= EndCardAtReal + TitleCardSeconds)
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
}
#include "Retake/StuntGuard.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Retake/RetakeAudioSet.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Retake/RetakeActivator.h"
#include "Retake/RetakeCharacter.h"
#include "Retake/RetakeSceneDirector.h"
#include "Retake/TakeGhost.h"

AStuntGuard::AStuntGuard()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	// A plain AIController lets AddMovementInput drive CharacterMovement; all logic is in Tick.
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 360.f, 0.f);
	Move->MaxWalkSpeed = PatrolSpeed;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(RootComponent);
	BodyMesh->SetStaticMesh(RetakeVisuals::LoadBasicShape(TEXT("Cylinder")));
	BodyMesh->SetRelativeScale3D(FVector(0.85f, 0.85f, 1.9f));
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	VisorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisorMesh"));
	VisorMesh->SetupAttachment(RootComponent);
	VisorMesh->SetStaticMesh(RetakeVisuals::LoadBasicShape(TEXT("Cube")));
	VisorMesh->SetRelativeLocation(FVector(38.f, 0.f, 60.f));
	VisorMesh->SetRelativeScale3D(FVector(0.15f, 0.6f, 0.15f));
	VisorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(RootComponent);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 125.f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(28.f);
	Label->SetTextRenderColor(FColor(255, 80, 60));
	Label->SetText(FText::FromString(TEXT("SECURITY")));
}

void AStuntGuard::BeginPlay()
{
	Super::BeginPlay();
	StartTransform = GetActorTransform();
	RetakeVisuals::Tint(BodyMesh, FLinearColor(0.15f, 0.2f, 0.45f));
	RetakeVisuals::Tint(VisorMesh, FLinearColor(1.f, 0.9f, 0.1f));
	// F1: same mannequin as the stunt doubles, in a dark-blue security uniform (the SECURITY sign stays).
	if (URetakeSettings::ApplyCharacterLook(this, BodyMesh, nullptr, false))
	{
		VisorMesh->SetVisibility(false);
		const URetakeSettings* S = GetDefault<URetakeSettings>();
		if (UMaterialInterface* Base = S ? S->CinematicMaterial.LoadSynchronous() : nullptr)
		{
			USkeletalMeshComponent* Skel = GetMesh();
			for (int32 i = 0; i < Skel->GetNumMaterials(); ++i)
			{
				UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, this);
				MID->SetVectorParameterValue(TEXT("BaseColorTint"), i == 0 ? FLinearColor(0.008f, 0.014f, 0.06f) : FLinearColor(0.015f, 0.016f, 0.03f) /* linear: deep navy */);
				Skel->SetMaterial(i, MID);
			}
		}
	}
	SetGuardState(EStuntGuardState::Patrol);
}

void AStuntGuard::ResetForTake()
{
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
	}
	SetActorLocationAndRotation(StartTransform.GetLocation(), StartTransform.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
	bHeadingToB = true;
	Target.Reset();
	IgnoredGhost.Reset();
	IgnoreTimeLeft = 0.f;
	StuckTimeLeft = 0.f;
	SetGuardState(EStuntGuardState::Patrol);
}

void AStuntGuard::SetReplayMode(bool bReplay)
{
	bReplayMode = bReplay;
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		if (bReplay)
		{
			Move->DisableMovement();
		}
		else
		{
			Move->SetMovementMode(MOVE_Walking);
		}
	}
}

void AStuntGuard::ApplyReplayExtra(uint8 Extra)
{
	const EStuntGuardState Recorded = static_cast<EStuntGuardState>(Extra);
	if (Recorded != GuardState)
	{
		SetGuardState(Recorded);
	}
}

void AStuntGuard::SetGuardState(EStuntGuardState NewState)
{
	GuardState = NewState;
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = (NewState == EStuntGuardState::Patrol) ? PatrolSpeed : ChaseSpeed;
	}
	switch (NewState)
	{
	case EStuntGuardState::Patrol: Label->SetText(FText::FromString(TEXT("SECURITY"))); break;
	case EStuntGuardState::Chase:  Label->SetText(FText::FromString(TEXT("HEY! STOP!"))); break;
	case EStuntGuardState::Stuck:  Label->SetText(FText::FromString(TEXT("GOTCHA!"))); break;
	}
}

static bool IsFinishedGhost(const AActor* Actor)
{
	const ATakeGhost* Ghost = Cast<ATakeGhost>(Actor);
	return Ghost && Ghost->HasFinished();
}

bool AStuntGuard::CanSee(const AActor* Candidate) const
{
	// A ghost whose take has ended is just a frozen prop: it can't keep the guard busy forever.
	if (!IsValid(Candidate) || !Candidate->GetActorEnableCollision() || Candidate->IsHidden() || IsFinishedGhost(Candidate))
	{
		return false;
	}
	const FVector ToTarget = Candidate->GetActorLocation() - GetActorLocation();
	if (ToTarget.SizeSquared() > FMath::Square(SightRange))
	{
		return false;
	}
	const float CosAngle = FVector::DotProduct(GetActorForwardVector(), ToTarget.GetSafeNormal());
	if (CosAngle < FMath::Cos(FMath::DegreesToRadians(SightHalfAngle)))
	{
		return false;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GuardSight), false, this);
	Params.AddIgnoredActor(Candidate);
	FHitResult Hit;
	const FVector Eye = GetActorLocation() + FVector(0.f, 0.f, 60.f);
	return !GetWorld()->LineTraceSingleByChannel(Hit, Eye, Candidate->GetActorLocation(), ECC_Visibility, Params);
}

AActor* AStuntGuard::FindVisibleTarget() const
{
	AActor* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	auto Consider = [&](AActor* Candidate)
	{
		if (Candidate == IgnoredGhost.Get() || !CanSee(Candidate))
		{
			return;
		}
		const float D = FVector::DistSquared(Candidate->GetActorLocation(), GetActorLocation());
		if (D < BestDistSq)
		{
			BestDistSq = D;
			Best = Candidate;
		}
	};
	for (TActorIterator<ARetakeCharacter> It(GetWorld()); It; ++It) { Consider(*It); }
	for (TActorIterator<ATakeGhost> It(GetWorld()); It; ++It) { Consider(*It); }
	return Best;
}

void AStuntGuard::MoveToward(const FVector& Location, float Speed)
{
	FVector Dir = Location - GetActorLocation();
	Dir.Z = 0.f;
	if (Dir.SizeSquared() > 1.f)
	{
		AddMovementInput(Dir.GetSafeNormal(), 1.f);
	}
}

void AStuntGuard::OnCaught(AActor* Caught)
{
	OnCaughtSomeone.Broadcast(this, Caught);
	if (Cast<ARetakeCharacter>(Caught))
	{
		if (ARetakeSceneDirector* Director = ARetakeSceneDirector::Get(this))
		{
			Director->BeginStuntDeath(ERetakeCutReason::Caught, GetActorLocation());
		}
		Target.Reset();
		SetGuardState(EStuntGuardState::Patrol);
		return;
	}
	// A ghost keeps replaying; the guard is stuck on it.
	Target = Caught;
	StuckTimeLeft = StuckSeconds;
	SetGuardState(EStuntGuardState::Stuck);
}

void AStuntGuard::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IgnoreTimeLeft > 0.f)
	{
		IgnoreTimeLeft -= DeltaSeconds;
		if (IgnoreTimeLeft <= 0.f)
		{
			IgnoredGhost.Reset();
		}
	}

	if (bReplayMode)
	{
		return; // the director plays back the recorded track
	}

	// Only act while a take is rolling (or the Director's Cut replays).
	if (const ARetakeSceneDirector* Director = ARetakeSceneDirector::Get(this))
	{
		const ERetakeSceneState S = Director->GetState();
		if (S != ERetakeSceneState::Rolling && S != ERetakeSceneState::DirectorsCut)
		{
			return;
		}
	}

	switch (GuardState)
	{
	case EStuntGuardState::Patrol:
	{
		if (AActor* Seen = FindVisibleTarget())
		{
			Target = Seen;
			SetGuardState(EStuntGuardState::Chase);
			OnSpotted.Broadcast(this, Seen);
			break;
		}
		const FVector A = StartTransform.GetLocation();
		const FVector B = StartTransform.TransformPosition(PatrolPointB);
		const FVector Goal = bHeadingToB ? B : A;
		if (FVector::Dist2D(GetActorLocation(), Goal) < 50.f)
		{
			bHeadingToB = !bHeadingToB;
		}
		MoveToward(Goal, PatrolSpeed);
		break;
	}
	case EStuntGuardState::Chase:
	{
		AActor* Current = Target.Get();
		// Re-evaluate: always chase the nearest visible actor.
		if (AActor* Seen = FindVisibleTarget())
		{
			Current = Seen;
			Target = Seen;
		}
		if (!IsValid(Current) || !Current->GetActorEnableCollision() || IsFinishedGhost(Current) ||
			FVector::Dist(Current->GetActorLocation(), GetActorLocation()) > SightRange * LoseSightMultiplier)
		{
			Target.Reset();
			SetGuardState(EStuntGuardState::Patrol);
			break;
		}
		const FVector Delta = Current->GetActorLocation() - GetActorLocation();
		if (Delta.Size2D() < CatchDistance && FMath::Abs(Delta.Z) < 150.f)
		{
			OnCaught(Current);
			break;
		}
		MoveToward(Current->GetActorLocation(), ChaseSpeed);
		break;
	}
	case EStuntGuardState::Stuck:
	{
		AActor* Ghost = Target.Get();
		StuckTimeLeft -= DeltaSeconds;
		if (IsValid(Ghost) && FVector::Dist2D(Ghost->GetActorLocation(), GetActorLocation()) > CatchDistance * 0.8f)
		{
			MoveToward(Ghost->GetActorLocation(), ChaseSpeed); // stay glued to the ghost
		}
		if (StuckTimeLeft <= 0.f || !IsValid(Ghost) || IsFinishedGhost(Ghost))
		{
			IgnoredGhost = Ghost;
			IgnoreTimeLeft = GhostIgnoreSeconds;
			Target.Reset();
			SetGuardState(EStuntGuardState::Patrol);
		}
		break;
	}
	}
}

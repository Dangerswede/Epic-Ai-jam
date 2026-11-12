#include "Retake/TakeGhost.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Retake/RetakeInteractable.h"
#include "UObject/ConstructorHelpers.h"

ATakeGhost::ATakeGhost()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// Same capsule as ARetakeCharacter.
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	AutoPossessAI = EAutoPossessAI::Disabled;
	AIControllerClass = nullptr;
	bUseControllerRotationYaw = false;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(RootComponent);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(CylinderMesh.Object);
		BodyMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.9f));
	}

	TakeLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TakeLabel"));
	TakeLabel->SetupAttachment(RootComponent);
	TakeLabel->SetRelativeLocation(FVector(0.f, 0.f, 125.f));
	TakeLabel->SetHorizontalAlignment(EHTA_Center);
	TakeLabel->SetVerticalAlignment(EVRTA_TextCenter);
	TakeLabel->SetWorldSize(32.f);
	TakeLabel->SetTextRenderColor(FColor(255, 220, 160));
	TakeLabel->SetText(FText::FromString(TEXT("TAKE ?")));
	TakeLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ATakeGhost::BeginPlay()
{
	Super::BeginPlay();

	// Frames drive the ghost; CharacterMovement must not simulate (no gravity, no floor snapping).
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->SetComponentTickEnabled(false);
		Move->bOrientRotationToMovement = false;
	}

	ApplyGhostMaterial();
}

void ATakeGhost::ApplyGhostMaterial()
{
	GhostMIDs.Reset();

	auto SetupMesh = [this](UMeshComponent* MeshComp)
	{
		if (!MeshComp)
		{
			return;
		}
		const int32 Num = MeshComp->GetNumMaterials();
		for (int32 i = 0; i < Num; ++i)
		{
			UMaterialInterface* Base = GhostMaterial ? GhostMaterial.Get() : MeshComp->GetMaterial(i);
			if (!Base)
			{
				continue;
			}
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, this);
			MID->SetVectorParameterValue(TintParameterName, TintColor);
			MID->SetScalarParameterValue(OpacityParameterName, Opacity);
			MeshComp->SetMaterial(i, MID);
			GhostMIDs.Add(MID);
		}
	};

	SetupMesh(BodyMesh);
	SetupMesh(GetMesh());
}

void ATakeGhost::Init(const FTakeData& InTake)
{
	Take = InTake;
	PlaybackTime = 0.f;
	LastActionTime = -1.f;
	NextActionFrame = 0;
	bPlaying = false;
	bFinished = false;

	TakeLabel->SetText(FText::FromString(FString::Printf(TEXT("TAKE %d"), Take.TakeNumber)));

	if (Take.Frames.Num() > 0)
	{
		ApplyFrameAt(0.f);
	}
}

void ATakeGhost::Play(float StartTime)
{
	if (Take.Frames.Num() == 0)
	{
		Finish();
		return;
	}

	PlaybackTime = FMath::Max(0.f, StartTime);
	bFinished = false;
	bPlaying = true;

	// Skip actions before the start time; fire the ones exactly at it.
	NextActionFrame = 0;
	while (NextActionFrame < Take.Frames.Num() && Take.Frames[NextActionFrame].Time < PlaybackTime)
	{
		++NextActionFrame;
	}

	ApplyFrameAt(PlaybackTime);
	FireActionsUpTo(PlaybackTime);
}

void ATakeGhost::Stop()
{
	bPlaying = false;
}

void ATakeGhost::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FaceLabelToCamera();

	if (!bPlaying)
	{
		return;
	}

	PlaybackTime += DeltaSeconds;

	if (PlaybackTime >= Take.Duration)
	{
		PlaybackTime = Take.Duration;
		ApplyFrameAt(PlaybackTime);
		FireActionsUpTo(PlaybackTime);
		Finish();
		return;
	}

	ApplyFrameAt(PlaybackTime);
	FireActionsUpTo(PlaybackTime);
}

void ATakeGhost::ApplyFrameAt(float Time)
{
	const TArray<FTakeFrame>& Frames = Take.Frames;
	if (Frames.Num() == 0)
	{
		return;
	}

	// Frames are at a fixed 30 Hz, so the index can be computed directly.
	const float Index = Time * 30.f;
	const int32 A = FMath::Clamp(FMath::FloorToInt(Index), 0, Frames.Num() - 1);
	const int32 B = FMath::Min(A + 1, Frames.Num() - 1);
	CurrentFrame = A;
	const float Alpha = (A == B) ? 0.f : FMath::Clamp(Index - A, 0.f, 1.f);

	const FTakeFrame& FA = Frames[A];
	const FTakeFrame& FB = Frames[B];

	const FVector Location = FMath::Lerp(FA.Location, FB.Location, Alpha);
	const FQuat Rotation = FQuat::Slerp(FA.Rotation.Quaternion(), FB.Rotation.Quaternion(), Alpha);
	const FVector Velocity = FMath::Lerp(FA.Velocity, FB.Velocity, Alpha);

	// No sweep: the recording is the truth. Overlaps (pressure plates, goal) still update.
	SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::None);

	// Feed the anim blueprint (speed + falling) without simulating movement.
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->Velocity = Velocity;
		const bool bAirborne = FMath::Abs(Velocity.Z) > 10.f;
		const EMovementMode Wanted = bAirborne ? MOVE_Falling : MOVE_Walking;
		if (Move->MovementMode != Wanted)
		{
			Move->SetMovementMode(Wanted);
		}
	}
}

void ATakeGhost::FireActionsUpTo(float Time)
{
	const TArray<FTakeFrame>& Frames = Take.Frames;
	while (NextActionFrame < Frames.Num() && Frames[NextActionFrame].Time <= Time + KINDA_SMALL_NUMBER)
	{
		const FTakeFrame& Frame = Frames[NextActionFrame];
		if (Frame.ActionFlags != 0)
		{
			LastActionTime = Frame.Time;
		}
		if (Frame.HasAction(ERetakeAction::Interact))
		{
			// Same code path as the player's Interact.
			RetakeInteraction::PerformInteract(this, InteractRadius);
		}
		// Jump: visual only (falling state comes from velocity). Push: crates react to the ghost's
		// recorded movement, nothing extra to fire.
		++NextActionFrame;
	}
}

bool ATakeGhost::IsActionActiveNow(ERetakeAction Action) const
{
	if (!bPlaying || Take.Frames.Num() == 0)
	{
		return false;
	}
	const int32 Last = Take.Frames.Num() - 1;
	const int32 A = FMath::Clamp(CurrentFrame, 0, Last);
	const int32 B = FMath::Min(A + 1, Last);
	return Take.Frames[A].HasAction(Action) || Take.Frames[B].HasAction(Action);
}

void ATakeGhost::Finish()
{
	bPlaying = false;
	if (bFinished)
	{
		return;
	}
	bFinished = true;

	// Freeze in the last pose (keeps standing on a pressure plate).
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->Velocity = FVector::ZeroVector;
		Move->SetMovementMode(MOVE_Walking);
	}

	// A take that ended in a stunt death: the stunt double stays down. No collision, so it
	// doesn't weigh down plates and the guard can't see it.
	if (Take.EndsInDeath())
	{
		BodyMesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
		BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -60.f));
		GetMesh()->SetRelativeRotation(GetMesh()->GetRelativeRotation() + FRotator(0.f, 0.f, 90.f));
		TakeLabel->SetRelativeLocation(FVector(0.f, 0.f, 20.f));
		SetActorEnableCollision(false);
	}
	OnFinished.Broadcast(this);
}

void ATakeGhost::FaceLabelToCamera()
{
	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC || !PC->PlayerCameraManager || !TakeLabel)
	{
		return;
	}
	const FVector ToCamera = PC->PlayerCameraManager->GetCameraLocation() - TakeLabel->GetComponentLocation();
	TakeLabel->SetWorldRotation(FRotator(0.f, ToCamera.Rotation().Yaw, 0.f));
}

// ---------------------------------------------------------------------------
// Debug console command: Retake.SpawnGhost  (until ARetakeSceneDirector exists)
// Spawns a ghost replaying the player's last finished take.
// ---------------------------------------------------------------------------
#include "EngineUtils.h"
#include "Retake/RetakeCharacter.h"
#include "Retake/RetakeRecorderComponent.h"

static void SpawnGhostFromLastTake(UWorld* World)
{
	if (!World)
	{
		return;
	}
	for (TActorIterator<ARetakeCharacter> It(World); It; ++It)
	{
		const URetakeRecorderComponent* Recorder = It->GetRecorder();
		if (!Recorder || Recorder->GetLastTake().Frames.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Retake.SpawnGhost: no finished take yet (press R first)."));
			continue;
		}
		const FTakeData& Take = Recorder->GetLastTake();
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ATakeGhost* Ghost = World->SpawnActor<ATakeGhost>(ATakeGhost::StaticClass(),
			Take.Frames[0].Location, Take.Frames[0].Rotation, Params);
		if (Ghost)
		{
			Ghost->Init(Take);
			Ghost->Play(0.f);
			UE_LOG(LogTemp, Display, TEXT("Retake.SpawnGhost: replaying take %d (%.2f s)."), Take.TakeNumber, Take.Duration);
		}
	}
}

static FAutoConsoleCommandWithWorld GRetakeSpawnGhostCmd(
	TEXT("Retake.SpawnGhost"),
	TEXT("Debug: spawn a ghost replaying the player's last take."),
	FConsoleCommandWithWorldDelegate::CreateStatic(&SpawnGhostFromLastTake));

#include "Retake/PushCrate.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Retake/RetakeActivator.h"
#include "Retake/RetakeCharacter.h"
#include "Retake/TakeGhost.h"

namespace
{
	constexpr float PawnRadius = 42.f;
	constexpr float ContactSlack = 12.f;
}

APushCrate::APushCrate()
{
	PrimaryActorTick.bCanEverTick = true;
	// Move after characters so we see this frame's positions.
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	CrateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrateMesh"));
	SetRootComponent(CrateMesh);
	CrateMesh->SetStaticMesh(RetakeVisuals::LoadBasicShape(TEXT("Cube")));
	CrateMesh->SetCollisionProfileName(TEXT("BlockAll"));
	CrateMesh->SetMobility(EComponentMobility::Movable);
	CrateMesh->SetSimulatePhysics(false);

	// Crates press pressure plates.
	Tags.Add(TEXT("RetakeWeight"));
}

void APushCrate::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// The engine cube is centered, 100 cm. Root scale sets the size.
	SetActorScale3D(FVector(Size / 100.f));
}

void APushCrate::BeginPlay()
{
	Super::BeginPlay();
	StartTransform = GetActorTransform();
	RetakeVisuals::Tint(CrateMesh, FLinearColor(0.55f, 0.35f, 0.15f));
}

void APushCrate::ResetForTake()
{
	SetActorTransform(StartTransform, false, nullptr, ETeleportType::TeleportPhysics);
}

bool APushCrate::GetContactFace(const AActor* Pusher, FVector& OutDir, float& OutAxisDist) const
{
	const float Half = Size * 0.5f;
	const FVector Local = GetActorTransform().InverseTransformPositionNoScale(Pusher->GetActorLocation());

	// Pusher must be vertically overlapping the crate (feet below the top).
	const float FeetZ = Local.Z - 96.f;
	if (FeetZ > Half - 10.f || Local.Z + 96.f < -Half)
	{
		return false;
	}

	const bool bXAxis = FMath::Abs(Local.X) >= FMath::Abs(Local.Y);
	const float Along = bXAxis ? Local.X : Local.Y;
	const float Across = bXAxis ? Local.Y : Local.X;
	if (FMath::Abs(Across) > Half + PawnRadius * 0.5f)
	{
		return false;
	}
	OutAxisDist = FMath::Abs(Along);
	if (OutAxisDist > Half + PawnRadius + ContactSlack)
	{
		return false;
	}
	// Push away from the pusher.
	const FVector LocalDir = bXAxis ? FVector(-FMath::Sign(Along), 0.f, 0.f) : FVector(0.f, -FMath::Sign(Along), 0.f);
	OutDir = GetActorQuat().RotateVector(LocalDir);
	return true;
}

void APushCrate::TryPushBy(AActor* Pusher, float DeltaSeconds)
{
	FVector Dir;
	float AxisDist = 0.f;
	if (!GetContactFace(Pusher, Dir, AxisDist))
	{
		return;
	}

	const float Contact = Size * 0.5f + PawnRadius;
	FVector Delta = FVector::ZeroVector;

	if (ARetakeCharacter* Player = Cast<ARetakeCharacter>(Pusher))
	{
		// Live player: needs to be steering into the face.
		const FVector Accel = Player->GetCharacterMovement()->GetCurrentAcceleration().GetSafeNormal2D();
		if (FVector::DotProduct(Accel, Dir) < 0.7f)
		{
			return;
		}
		Delta = Dir * PushSpeed * DeltaSeconds;
		Player->NotifyAction(ERetakeAction::Push);
	}
	else if (const ATakeGhost* Ghost = Cast<ATakeGhost>(Pusher))
	{
		// Ghost: only on recorded Push frames; follow the recorded position so the result matches the take.
		if (!Ghost->IsActionActiveNow(ERetakeAction::Push))
		{
			return;
		}
		const float Wanted = Contact - AxisDist; // how far the crate must move to sit just in front
		if (Wanted <= 0.f)
		{
			return;
		}
		Delta = Dir * FMath::Min(Wanted, PushSpeed * 2.f * DeltaSeconds);
	}
	else
	{
		return;
	}

	FHitResult Hit;
	AddActorWorldOffset(Delta, true, &Hit, ETeleportType::None);
}

void APushCrate::ApplyFall(float DeltaSeconds)
{
	const float Half = Size * 0.5f;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CrateFloor), false, this);
	FHitResult Floor;
	const FVector Start = GetActorLocation();
	const FVector End = Start - FVector(0.f, 0.f, 10000.f);
	const float FeetZ = Start.Z - Half;
	float GroundZ = -1e9f;
	if (GetWorld()->LineTraceSingleByChannel(Floor, Start, End, ECC_WorldStatic, Params))
	{
		GroundZ = Floor.ImpactPoint.Z;
	}
	if (FeetZ <= GroundZ + 1.f)
	{
		return; // supported
	}
	// Center is over a gap: drop straight down (no sweep so the ledge corner can't snag it), snap on landing.
	const float Drop = FMath::Min(FallSpeed * DeltaSeconds, FeetZ - GroundZ);
	AddActorWorldOffset(FVector(0.f, 0.f, -Drop), false, nullptr, ETeleportType::None);
}

void APushCrate::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	for (TActorIterator<ARetakeCharacter> It(GetWorld()); It; ++It)
	{
		if (It->GetActorEnableCollision())
		{
			TryPushBy(*It, DeltaSeconds);
		}
	}
	for (TActorIterator<ATakeGhost> It(GetWorld()); It; ++It)
	{
		TryPushBy(*It, DeltaSeconds);
	}
	ApplyFall(DeltaSeconds);
}

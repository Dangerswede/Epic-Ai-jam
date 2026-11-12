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
	// WorldDynamic so pressure-plate meshes can let it slide over their lip; overlaps so plates notice it.
	CrateMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CrateMesh->SetGenerateOverlapEvents(true);
	CrateMesh->SetMobility(EComponentMobility::Movable);
	CrateMesh->SetSimulatePhysics(false);

	// Crates press pressure plates.
	Tags.Add(TEXT("RetakeWeight"));
}

void APushCrate::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// The engine cube is centered, 100 cm. Root scale sets the size.
	SetActorScale3D(Size / 100.f);
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
	const FVector Half = Size * 0.5f;
	const FVector Local = GetActorTransform().InverseTransformPositionNoScale(Pusher->GetActorLocation());

	// Pusher must be vertically overlapping the crate (feet below the top).
	const float FeetZ = Local.Z - 96.f;
	if (FeetZ > Half.Z - 10.f || Local.Z + 96.f < -Half.Z)
	{
		return false;
	}

	// Pick the face the pusher is closest to, relative to the crate's proportions.
	const bool bXAxis = (FMath::Abs(Local.X) - Half.X) >= (FMath::Abs(Local.Y) - Half.Y);
	const float Along = bXAxis ? Local.X : Local.Y;
	const float Across = bXAxis ? Local.Y : Local.X;
	const float HalfAlong = bXAxis ? Half.X : Half.Y;
	const float HalfAcross = bXAxis ? Half.Y : Half.X;
	if (FMath::Abs(Across) > HalfAcross + PawnRadius * 0.5f)
	{
		return false;
	}
	OutAxisDist = FMath::Abs(Along) - HalfAlong; // distance from the face
	if (OutAxisDist > PawnRadius + ContactSlack)
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
		const float Wanted = PawnRadius - AxisDist; // how far the crate must move to sit just in front
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
	const FVector Before = GetActorLocation();
	AddActorWorldOffset(Delta, true, &Hit, ETeleportType::None);
	if (!GetActorLocation().Equals(Before, 0.1f))
	{
		if (StillTime > 0.3f)
		{
			OnPushStarted.Broadcast(this);
		}
		StillTime = 0.f;
	}
}

void APushCrate::ApplyFall(float DeltaSeconds)
{
	const FVector Half = Size * 0.5f;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CrateFloor), false, this);
	const FTransform Xf = GetActorTransform();
	const float BottomZ = GetActorLocation().Z - Half.Z;

	// Trace down from the center and the four (slightly inset) bottom corners; highest ground supports the crate.
	const FVector2D Points[] = {
		{ 0.f, 0.f },
		{ Half.X - 5.f, Half.Y - 5.f }, { -Half.X + 5.f, Half.Y - 5.f },
		{ Half.X - 5.f, -Half.Y + 5.f }, { -Half.X + 5.f, -Half.Y + 5.f },
	};
	float GroundZ = -1e9f;
	for (const FVector2D& P : Points)
	{
		const FVector Start = Xf.TransformPositionNoScale(FVector(P.X, P.Y, 0.f));
		const FVector End = Start - FVector(0.f, 0.f, 10000.f);
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
		{
			GroundZ = FMath::Max(GroundZ, Hit.ImpactPoint.Z);
		}
	}
	if (BottomZ <= GroundZ + 1.f)
	{
		return; // supported
	}
	// No support: drop straight down (no sweep so a ledge corner can't snag it), snap on landing.
	const float Drop = FMath::Min(FallSpeed * DeltaSeconds, BottomZ - GroundZ);
	AddActorWorldOffset(FVector(0.f, 0.f, -Drop), false, nullptr, ETeleportType::None);
}
void APushCrate::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bReplayMode)
	{
		return; // the director plays back the recorded track
	}
	StillTime += DeltaSeconds;

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

#include "Retake/PressurePlate.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"

APressurePlate::APressurePlate()
{
	PrimaryActorTick.bCanEverTick = true; // only while the press animation runs
	PrimaryActorTick.bStartWithTickEnabled = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->SetBoxExtent(FVector(75.f, 75.f, 40.f));
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	Trigger->SetGenerateOverlapEvents(true);

	PlateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlateMesh"));
	PlateMesh->SetupAttachment(Trigger);
	PlateMesh->SetStaticMesh(RetakeVisuals::LoadBasicShape(TEXT("Cylinder")));
	PlateMesh->SetRelativeLocation(FVector(0.f, 0.f, -35.f));
	PlateMesh->SetRelativeScale3D(FVector(1.4f, 1.4f, 0.1f));
	PlateMesh->SetMobility(EComponentMobility::Movable);
	PlateMesh->SetCollisionProfileName(TEXT("BlockAll"));
	PlateMesh->SetGenerateOverlapEvents(false);
	// Crates (WorldDynamic) slide over the ~10 cm plate instead of catching on its edge.
	PlateMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
	PlateMesh->SetCanEverAffectNavigation(false);
	ArtMesh = RetakeVisuals::CreateArtComponent(this, PlateMesh);
}

void APressurePlate::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RetakeVisuals::ApplyArt(PlateMesh, ArtMesh, ArtMeshAsset, ArtSize);
}

void APressurePlate::BeginPlay()
{
	Super::BeginPlay();
	PlateRestLocation = PlateMesh->GetRelativeLocation();
	if (ArtMesh)
	{
		ArtBaseScale = ArtMesh->GetComponentScale();
	}
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &APressurePlate::HandleOverlapChanged);
	Trigger->OnComponentEndOverlap.AddDynamic(this, &APressurePlate::HandleEndOverlap);
	OnActiveChanged();
	Reevaluate();
	bVisualsReady = true;
}

bool APressurePlate::CountsAsWeight(const AActor* Actor) const
{
	if (!IsValid(Actor) || Actor == this || Actor->IsActorBeingDestroyed())
	{
		return false;
	}
	// Player and ghosts are both ACharacter. Hidden/collisionless player (Director's Cut) doesn't count.
	if (Actor->IsA<ACharacter>())
	{
		return Actor->GetActorEnableCollision();
	}
	return WeightTag != NAME_None && Actor->ActorHasTag(WeightTag);
}

void APressurePlate::Reevaluate()
{
	TArray<AActor*> Overlapping;
	Trigger->GetOverlappingActors(Overlapping);
	bool bAny = false;
	for (const AActor* Actor : Overlapping)
	{
		if (CountsAsWeight(Actor))
		{
			bAny = true;
			break;
		}
	}
	SetActive(bAny);
}

void APressurePlate::HandleOverlapChanged(UPrimitiveComponent*, AActor*, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	Reevaluate();
}

void APressurePlate::HandleEndOverlap(UPrimitiveComponent*, AActor*, UPrimitiveComponent*, int32)
{
	Reevaluate();
}

void APressurePlate::ResetForTake()
{
	Reevaluate();
}

void APressurePlate::OnActiveChanged()
{
	if (!PlateMesh)
	{
		return;
	}
	if (!bVisualsReady)
	{
		// Initial state: snap, no animation.
		PressAlpha = IsActive() ? 1.f : 0.f;
		PressVel = 0.f;
		Flash = 0.f;
		ApplyPressVisuals();
		return;
	}
	if (IsActive())
	{
		Flash = 1.f;
		PressVel += 6.f; // thump: overshoots below the pressed depth, then settles
	}
	else
	{
		PressVel -= 4.f; // pops back up past the rest height
	}
	SetActorTickEnabled(true);
}

void APressurePlate::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Target = IsActive() ? 1.f : 0.f;
	RetakeVisuals::SpringStep(PressAlpha, PressVel, Target, DeltaSeconds, 420.f, 16.f);
	Flash = FMath::FInterpTo(Flash, 0.f, DeltaSeconds, 6.f);
	if (FMath::Abs(PressAlpha - Target) < 0.002f && FMath::Abs(PressVel) < 0.01f && Flash < 0.01f)
	{
		PressAlpha = Target;
		PressVel = 0.f;
		Flash = 0.f;
		SetActorTickEnabled(false);
	}
	ApplyPressVisuals();
}

void APressurePlate::ApplyPressVisuals()
{
	if (!PlateMesh)
	{
		return;
	}
	const float A = FMath::Clamp(PressAlpha, 0.f, 1.f);
	RetakeVisuals::Tint(PlateMesh, FMath::Lerp(IdleColor, PressedColor, A) * (1.f + 2.5f * Flash));
	PlateMesh->SetRelativeLocation(PlateRestLocation - FVector(0.f, 0.f, PressedDepth * PressAlpha));
	// Art prop: squashes flat while pressed and bulges a touch on the thump.
	if (ArtMesh && ArtMesh->GetStaticMesh())
	{
		const float Bulge = 1.f + 0.05f * Flash;
		ArtMesh->SetWorldScale3D(ArtBaseScale * FVector(Bulge, Bulge, FMath::Max(0.2f, 1.f - 0.4f * PressAlpha)));
	}
}

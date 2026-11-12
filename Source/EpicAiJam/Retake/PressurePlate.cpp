#include "Retake/PressurePlate.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"

APressurePlate::APressurePlate()
{
	PrimaryActorTick.bCanEverTick = false;

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
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &APressurePlate::HandleOverlapChanged);
	Trigger->OnComponentEndOverlap.AddDynamic(this, &APressurePlate::HandleEndOverlap);
	OnActiveChanged();
	Reevaluate();
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
	RetakeVisuals::Tint(PlateMesh, IsActive() ? PressedColor : IdleColor);
	PlateMesh->SetRelativeLocation(PlateRestLocation - FVector(0.f, 0.f, IsActive() ? PressedDepth : 0.f));
}

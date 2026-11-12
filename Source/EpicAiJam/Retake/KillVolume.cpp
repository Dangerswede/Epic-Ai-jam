#include "Retake/KillVolume.h"

#include "Components/BoxComponent.h"
#include "Retake/RetakeCharacter.h"
#include "Retake/RetakeSceneDirector.h"

AKillVolume::AKillVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->SetBoxExtent(FVector(500.f, 500.f, 50.f));
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	Trigger->SetGenerateOverlapEvents(true);
	Trigger->ShapeColor = FColor::Red;
}

void AKillVolume::BeginPlay()
{
	Super::BeginPlay();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AKillVolume::HandleBeginOverlap);
}

void AKillVolume::HandleBeginOverlap(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	// Only the live player; ATakeGhost is not an ARetakeCharacter.
	if (!Cast<ARetakeCharacter>(Other))
	{
		return;
	}
	if (ARetakeSceneDirector* Director = ARetakeSceneDirector::Get(this))
	{
		Director->NotifyPlayerKilled();
	}
}

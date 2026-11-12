#include "Retake/GoalZone.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Retake/RetakeActivator.h"
#include "Retake/RetakeSceneDirector.h"

AGoalZone::AGoalZone()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->SetBoxExtent(FVector(100.f, 100.f, 100.f));
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	Trigger->SetGenerateOverlapEvents(true);

	MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
	MarkerMesh->SetupAttachment(Trigger);
	MarkerMesh->SetStaticMesh(RetakeVisuals::LoadBasicShape(TEXT("Cylinder")));
	MarkerMesh->SetRelativeLocation(FVector(0.f, 0.f, -98.f));
	MarkerMesh->SetRelativeScale3D(FVector(1.8f, 1.8f, 0.02f));
	MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGoalZone::BeginPlay()
{
	Super::BeginPlay();
	RetakeVisuals::Tint(MarkerMesh, MarkerColor);
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AGoalZone::HandleBeginOverlap);
}

void AGoalZone::HandleBeginOverlap(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (ARetakeSceneDirector* Director = ARetakeSceneDirector::Get(this))
	{
		Director->NotifyGoalReached(Other); // director ignores anything but the player
	}
}

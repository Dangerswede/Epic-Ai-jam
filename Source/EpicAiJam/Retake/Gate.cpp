#include "Retake/Gate.h"

#include "Components/StaticMeshComponent.h"
#include "Retake/RetakeActivator.h"

AGate::AGate()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(Root);
	DoorMesh->SetStaticMesh(RetakeVisuals::LoadBasicShape(TEXT("Cube")));
	// 300 wide (Y), 20 thick (X), 250 tall, bottom at the actor origin.
	DoorMesh->SetRelativeScale3D(FVector(0.2f, 3.f, 2.5f));
	DoorMesh->SetRelativeLocation(FVector(0.f, 0.f, 125.f));
	DoorMesh->SetMobility(EComponentMobility::Movable);
	DoorMesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void AGate::BeginPlay()
{
	Super::BeginPlay();
	ClosedLocation = DoorMesh->GetRelativeLocation();
	RetakeVisuals::Tint(DoorMesh, DoorColor);
	OpenAlpha = ShouldBeOpen() ? 1.f : 0.f;
	ApplyAlpha();
}

bool AGate::ShouldBeOpen() const
{
	for (const ARetakeActivator* Activator : LinkedActivators)
	{
		if (Activator && Activator->IsActive())
		{
			return true;
		}
	}
	return false;
}

void AGate::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Polling the few linked activators is simpler and order-independent compared to events.
	const float Target = ShouldBeOpen() ? 1.f : 0.f;
	if (!FMath::IsNearlyEqual(OpenAlpha, Target))
	{
		OpenAlpha = FMath::FInterpConstantTo(OpenAlpha, Target, DeltaSeconds, 1.f / MoveTime);
		ApplyAlpha();
	}
}

void AGate::ResetForTake()
{
	// Every take starts with the gate in its rest state.
	OpenAlpha = 0.f;
	ApplyAlpha();
}

void AGate::ApplyAlpha()
{
	const float Smooth = FMath::SmoothStep(0.f, 1.f, OpenAlpha);
	DoorMesh->SetRelativeLocation(ClosedLocation + OpenOffset * Smooth);
}

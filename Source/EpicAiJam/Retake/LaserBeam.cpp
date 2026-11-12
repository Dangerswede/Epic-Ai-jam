#include "Retake/LaserBeam.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Retake/RetakeActivator.h"
#include "Retake/RetakeCharacter.h"
#include "Retake/RetakeSceneDirector.h"

ALaserBeam::ALaserBeam()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(Root);
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	Trigger->SetGenerateOverlapEvents(true);

	BeamMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeamMesh"));
	BeamMesh->SetupAttachment(Root);
	BeamMesh->SetStaticMesh(RetakeVisuals::LoadBasicShape(TEXT("Cylinder")));
	BeamMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BeamMesh->SetCastShadow(false);
	BeamMesh->SetMobility(EComponentMobility::Movable);

	EmitterA = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EmitterA"));
	EmitterA->SetupAttachment(Root);
	EmitterA->SetStaticMesh(RetakeVisuals::LoadBasicShape(TEXT("Cube")));
	EmitterA->SetRelativeScale3D(FVector(0.2f));
	EmitterA->SetCollisionProfileName(TEXT("BlockAll"));
	EmitterA->SetMobility(EComponentMobility::Movable);

	EmitterB = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EmitterB"));
	EmitterB->SetupAttachment(Root);
	EmitterB->SetStaticMesh(RetakeVisuals::LoadBasicShape(TEXT("Cube")));
	EmitterB->SetRelativeScale3D(FVector(0.2f));
	EmitterB->SetCollisionProfileName(TEXT("BlockAll"));
	EmitterB->SetMobility(EComponentMobility::Movable);

	UpdateShape();
}

void ALaserBeam::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateShape();
}

void ALaserBeam::UpdateShape()
{
	const float Half = BeamLength * 0.5f;
	Trigger->SetRelativeLocation(FVector(Half, 0.f, 0.f));
	Trigger->SetBoxExtent(FVector(Half, 8.f, 8.f));
	// Engine cylinder is 100 tall along Z; lay it along X.
	BeamMesh->SetRelativeLocation(FVector(Half, 0.f, 0.f));
	BeamMesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	BeamMesh->SetRelativeScale3D(FVector(0.04f, 0.04f, BeamLength / 100.f));
	EmitterA->SetRelativeLocation(FVector::ZeroVector);
	EmitterB->SetRelativeLocation(FVector(BeamLength, 0.f, 0.f));
}

void ALaserBeam::BeginPlay()
{
	Super::BeginPlay();
	RetakeVisuals::Tint(BeamMesh, FLinearColor(3.f, 0.05f, 0.05f));
	RetakeVisuals::Tint(EmitterA, FLinearColor(0.1f, 0.1f, 0.1f));
	RetakeVisuals::Tint(EmitterB, FLinearColor(0.1f, 0.1f, 0.1f));
	SetBeamOn(!ARetakeActivator::AnyActive(LinkedActivators));
}

void ALaserBeam::SetBeamOn(bool bOn)
{
	bBeamOn = bOn;
	BeamMesh->SetVisibility(bOn);
}

void ALaserBeam::ResetForTake()
{
	SetBeamOn(!ARetakeActivator::AnyActive(LinkedActivators));
}

void ALaserBeam::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const bool bShouldBeOn = !ARetakeActivator::AnyActive(LinkedActivators);
	if (bShouldBeOn != bBeamOn)
	{
		SetBeamOn(bShouldBeOn);
	}
	if (!bBeamOn)
	{
		return;
	}

	// Polling handles "player already inside when the beam turns back on".
	TArray<AActor*> Overlapping;
	Trigger->GetOverlappingActors(Overlapping, ARetakeCharacter::StaticClass());
	for (AActor* Actor : Overlapping)
	{
		if (Actor->GetActorEnableCollision())
		{
			if (ARetakeSceneDirector* Director = ARetakeSceneDirector::Get(this))
			{
				// Thrown back the way they came: origin = closest point on the beam, nudged through to the far side.
				const FVector Start = GetActorLocation();
				const FVector Dir = GetActorForwardVector();
				const FVector OnBeam = Start + Dir * FMath::Clamp(FVector::DotProduct(Actor->GetActorLocation() - Start, Dir), 0.f, BeamLength);
				const FVector Push = Actor->GetVelocity().GetSafeNormal2D();
				Director->BeginStuntDeath(ERetakeCutReason::Killed, Push.IsNearlyZero() ? OnBeam : OnBeam + Push * 100.f);
			}
			break;
		}
	}
}

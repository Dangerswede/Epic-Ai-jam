#include "Retake/Lever.h"

#include "Components/StaticMeshComponent.h"

ALever::ALever()
{
	PrimaryActorTick.bCanEverTick = false;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	SetRootComponent(BaseMesh);
	BaseMesh->SetStaticMesh(RetakeVisuals::LoadBasicShape(TEXT("Cube")));
	BaseMesh->SetRelativeScale3D(FVector(0.4f, 0.4f, 0.3f));
	BaseMesh->SetCollisionProfileName(TEXT("BlockAll"));

	HandleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandleMesh"));
	HandleMesh->SetupAttachment(BaseMesh);
	HandleMesh->SetStaticMesh(RetakeVisuals::LoadBasicShape(TEXT("Cylinder")));
	// Relative to the scaled base: ~10 cm thick, ~80 cm tall stick on top.
	HandleMesh->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	HandleMesh->SetRelativeScale3D(FVector(0.25f, 0.25f, 2.5f));
	HandleMesh->SetMobility(EComponentMobility::Movable);
	HandleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ALever::BeginPlay()
{
	Super::BeginPlay();
	SetActive(bStartsActive);
	OnActiveChanged();
}

void ALever::RetakeInteract(AActor* Interactor)
{
	SetActive(!IsActive());
}

void ALever::ResetForTake()
{
	SetActive(bStartsActive);
}

void ALever::OnActiveChanged()
{
	if (!HandleMesh)
	{
		return;
	}
	HandleMesh->SetRelativeRotation(FRotator(IsActive() ? -HandleAngle : HandleAngle, 0.f, 0.f));
	RetakeVisuals::Tint(HandleMesh, IsActive() ? OnColor : OffColor);
}

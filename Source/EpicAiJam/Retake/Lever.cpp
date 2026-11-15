#include "Retake/Lever.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Retake/RetakeInteractable.h"

ALever::ALever()
{
	PrimaryActorTick.bCanEverTick = true; // handle swing + [E] prompt pulse

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	SetRootComponent(BaseMesh);
	BaseMesh->SetStaticMesh(RetakeVisuals::LoadBasicShape(TEXT("Cube")));
	BaseMesh->SetRelativeScale3D(FVector(0.4f, 0.4f, 0.3f));
	BaseMesh->SetCollisionProfileName(TEXT("BlockAll"));

	ArtMesh = RetakeVisuals::CreateArtComponent(this, BaseMesh);

	HandleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandleMesh"));
	HandleMesh->SetupAttachment(BaseMesh);
	HandleMesh->SetStaticMesh(RetakeVisuals::LoadBasicShape(TEXT("Cylinder")));
	// Relative to the scaled base: ~10 cm thick, ~80 cm tall stick on top.
	HandleMesh->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	HandleMesh->SetRelativeScale3D(FVector(0.25f, 0.25f, 2.5f));
	HandleMesh->SetMobility(EComponentMobility::Movable);
	HandleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	UseLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("UseLabel"));
	UseLabel->SetupAttachment(BaseMesh);
	UseLabel->SetUsingAbsoluteScale(true);
	UseLabel->SetRelativeLocation(FVector(0.f, 0.f, 420.f)); // ~125 cm above the base (base is scaled 0.3 in Z)
	UseLabel->SetHorizontalAlignment(EHTA_Center);
	UseLabel->SetWorldSize(40.f);
	UseLabel->SetTextRenderColor(FColor(255, 200, 0));
	UseLabel->SetText(FText::FromString(TEXT("[E]")));
	UseLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ALever::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// The graybox handle stays visible: it shows the on/off state.
	RetakeVisuals::ApplyArt(BaseMesh, ArtMesh, ArtMeshAsset, ArtSize);
}

void ALever::BeginPlay()
{
	Super::BeginPlay();
	SetActive(bStartsActive);
	OnActiveChanged();
	bVisualsReady = true;
}

void ALever::RetakeInteract(AActor* Interactor)
{
	SetActive(!IsActive());
	LabelPunch = 1.f;
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
	if (!bVisualsReady)
	{
		HandleT = IsActive() ? 1.f : 0.f; // initial state: snap
		HandleVel = 0.f;
		ApplyHandleVisuals();
	}
	// Otherwise Tick swings the handle over.
}

void ALever::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Clock += DeltaSeconds;
	const float Target = IsActive() ? 1.f : 0.f;
	if (FMath::Abs(HandleT - Target) > 0.001f || FMath::Abs(HandleVel) > 0.01f)
	{
		RetakeVisuals::SpringStep(HandleT, HandleVel, Target, DeltaSeconds, 700.f, 14.f);
		ApplyHandleVisuals();
	}
	if (UseLabel)
	{
		const bool bNear = RetakeVisuals::IsPlayerNear(this, RetakeInteraction::DefaultRadius + 60.f);
		LabelScale = FMath::FInterpTo(LabelScale, bNear ? 1.35f : 1.f, DeltaSeconds, 10.f);
		LabelPunch = FMath::FInterpTo(LabelPunch, 0.f, DeltaSeconds, 8.f);
		const float Pulse = bNear ? 1.f + 0.06f * FMath::Sin(Clock * 7.f) : 1.f;
		UseLabel->SetWorldSize(40.f * LabelScale * Pulse * (1.f + 0.5f * LabelPunch));
		UseLabel->SetTextRenderColor(bNear ? FColor::White : FColor(255, 200, 0));
	}
}

void ALever::ApplyHandleVisuals()
{
	HandleMesh->SetRelativeRotation(FRotator(FMath::Lerp(HandleAngle, -HandleAngle, HandleT), 0.f, 0.f));
	RetakeVisuals::Tint(HandleMesh, FMath::Lerp(OffColor, OnColor, FMath::Clamp(HandleT, 0.f, 1.f)));
}

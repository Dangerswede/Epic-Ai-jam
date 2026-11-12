#include "Retake/TimedSwitch.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"

ATimedSwitch::ATimedSwitch()
{
	PrimaryActorTick.bCanEverTick = true;

	PostMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PostMesh"));
	SetRootComponent(PostMesh);
	PostMesh->SetStaticMesh(RetakeVisuals::LoadBasicShape(TEXT("Cube")));
	PostMesh->SetRelativeScale3D(FVector(0.4f, 0.4f, 1.1f)); // 40 x 40 x 110 cm post
	PostMesh->SetCollisionProfileName(TEXT("BlockAll"));

	ArtMesh = RetakeVisuals::CreateArtComponent(this, PostMesh);

	TimerBar = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TimerBar"));
	TimerBar->SetupAttachment(PostMesh);
	TimerBar->SetStaticMesh(RetakeVisuals::LoadBasicShape(TEXT("Cube")));
	TimerBar->SetUsingAbsoluteScale(true);
	TimerBar->SetRelativeLocation(FVector(0.f, 0.f, 58.f)); // on top of the post (local units are scaled)
	TimerBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TimerBar->SetMobility(EComponentMobility::Movable);
	TimerBar->SetCastShadow(false);

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(PostMesh);
	Label->SetUsingAbsoluteScale(true);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(30.f);
	Label->SetTextRenderColor(FColor(255, 200, 0));
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	DisplayName = FText::FromString(TEXT("SWITCH"));
}

void ATimedSwitch::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RetakeVisuals::ApplyArt(PostMesh, ArtMesh, ArtMeshAsset, ArtSize);
	UpdateVisuals();
}

void ATimedSwitch::BeginPlay()
{
	Super::BeginPlay();
	RetakeVisuals::Tint(PostMesh, FLinearColor(0.2f, 0.2f, 0.22f));
	UpdateVisuals();
}

void ATimedSwitch::RetakeInteract(AActor* Interactor)
{
	TimeLeft = ActiveSeconds; // pressing again restarts the window
	SetActive(true);
	UpdateVisuals();
}

void ATimedSwitch::ResetForTake()
{
	TimeLeft = 0.f;
	SetActive(false);
	UpdateVisuals();
}

void ATimedSwitch::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!IsActive())
	{
		return;
	}
	TimeLeft -= DeltaSeconds;
	if (TimeLeft <= 0.f)
	{
		TimeLeft = 0.f;
		SetActive(false);
	}
	UpdateVisuals();
}

void ATimedSwitch::OnActiveChanged()
{
	UpdateVisuals();
}

void ATimedSwitch::UpdateVisuals()
{
	if (!TimerBar || !Label)
	{
		return;
	}
	const float Alpha = IsActive() ? FMath::Clamp(TimeLeft / FMath::Max(0.01f, ActiveSeconds), 0.f, 1.f) : 1.f;
	// 50 cm wide bar on top; shrinks as the window closes.
	TimerBar->SetWorldScale3D(FVector(0.5f * FMath::Max(0.05f, Alpha), 0.12f, 0.12f));
	RetakeVisuals::Tint(TimerBar, IsActive() ? FLinearColor(0.1f, 1.f, 0.2f) * 2.f : FLinearColor(0.8f, 0.1f, 0.08f));
	const FString Name = DisplayName.ToString();
	Label->SetText(FText::FromString(IsActive()
		? FString::Printf(TEXT("%s  %.1f"), *Name, TimeLeft)
		: FString::Printf(TEXT("%s  [E]"), *Name)));
	Label->SetTextRenderColor(IsActive() ? FColor(80, 255, 120) : FColor(255, 200, 0));
}

#include "Retake/TimedSwitch.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Retake/RetakeInteractable.h"

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
	if (ArtMesh)
	{
		ArtBaseScale = ArtMesh->GetComponentScale();
	}
	UpdateVisuals();
}

void ATimedSwitch::RetakeInteract(AActor* Interactor)
{
	TimeLeft = ActiveSeconds; // pressing again restarts the window
	Punch = 1.f;
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
	Clock += DeltaSeconds;
	Punch = FMath::FInterpTo(Punch, 0.f, DeltaSeconds, 7.f);
	const bool bNear = !IsActive() && RetakeVisuals::IsPlayerNear(this, RetakeInteraction::DefaultRadius + 60.f);
	LabelScale = FMath::FInterpTo(LabelScale, bNear ? 1.3f : 1.f, DeltaSeconds, 10.f);
	if (IsActive())
	{
		TimeLeft -= DeltaSeconds;
		if (TimeLeft <= 0.f)
		{
			TimeLeft = 0.f;
			SetActive(false);
		}
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
	// Press punch: the bar jumps thick and the art prop squashes, then both spring back.
	const float Thick = 0.12f * (1.f + 0.8f * Punch);
	TimerBar->SetWorldScale3D(FVector(0.5f * FMath::Max(0.05f, Alpha) * (1.f + 0.2f * Punch), Thick, Thick));
	// Last 1.5 s: the bar blinks orange so the window closing is readable from afar.
	const bool bLow = IsActive() && TimeLeft < 1.5f;
	const bool bBlinkOn = bLow && FMath::Fmod(Clock * 8.f, 2.f) < 1.f;
	const FLinearColor BarColor = !IsActive() ? FLinearColor(0.8f, 0.1f, 0.08f)
		: bBlinkOn ? FLinearColor(1.f, 0.45f, 0.05f) * 2.5f
		: FLinearColor(0.1f, 1.f, 0.2f) * (2.f + 3.f * Punch);
	RetakeVisuals::Tint(TimerBar, BarColor);
	if (HasActorBegunPlay() && ArtMesh && ArtMesh->GetStaticMesh()) // ArtBaseScale is read in BeginPlay
	{
		const float Squash = 0.12f * Punch;
		ArtMesh->SetWorldScale3D(ArtBaseScale * FVector(1.f + 0.5f * Squash, 1.f + 0.5f * Squash, 1.f - Squash));
	}
	Label->SetWorldSize(30.f * LabelScale * (LabelScale > 1.01f ? 1.f + 0.06f * FMath::Sin(Clock * 7.f) : 1.f) * (1.f + 0.4f * Punch));
	const FString Name = DisplayName.ToString();
	Label->SetText(FText::FromString(IsActive()
		? FString::Printf(TEXT("%s  %.1f"), *Name, TimeLeft)
		: FString::Printf(TEXT("%s  [E]"), *Name)));
	Label->SetTextRenderColor(IsActive() ? (bBlinkOn ? FColor(255, 140, 20) : FColor(80, 255, 120))
		: (LabelScale > 1.05f ? FColor::White : FColor(255, 200, 0)));
}

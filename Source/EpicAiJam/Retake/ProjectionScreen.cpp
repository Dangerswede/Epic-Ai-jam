#include "Retake/ProjectionScreen.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Texture2D.h"
#include "Retake/RetakeSceneDirector.h"
#include "Retake/RetakeScenes.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Retake/RetakeActivator.h"
#include "UObject/ConstructorHelpers.h"

AProjectionScreen::AProjectionScreen()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Frame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Frame"));
	Frame->SetupAttachment(Root);
	Frame->SetStaticMesh(RetakeVisuals::LoadBasicShape(TEXT("Cube")));
	Frame->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ScreenMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScreenMesh"));
	ScreenMesh->SetupAttachment(Root);
	ScreenMesh->SetStaticMesh(RetakeVisuals::LoadBasicShape(TEXT("Cube"))); // a thin slab: visible whichever way it faces
	ScreenMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ScreenMesh->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ScreenMat(TEXT("/Game/Retake/FX/M_ProjectionScreen.M_ProjectionScreen"));
	if (ScreenMat.Succeeded())
	{
		ScreenMesh->SetMaterial(0, ScreenMat.Object);
	}

	Caption = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Caption"));
	Caption->SetupAttachment(Root);
	Caption->SetRelativeLocation(FVector(8.f, 0.f, 0.f));
	Caption->SetHorizontalAlignment(EHTA_Center);
	Caption->SetVerticalAlignment(EVRTA_TextCenter);
	Caption->SetWorldSize(34.f);
	Caption->SetTextRenderColor(FColor(40, 28, 18));
	Caption->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
	Capture->SetupAttachment(Root);
	Capture->SetUsingAbsoluteLocation(true);
	Capture->SetUsingAbsoluteRotation(true);
	Capture->bCaptureEveryFrame = false; // driven from Tick at ~20 fps (old projector, and cheaper)
	Capture->bCaptureOnMovement = false;
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	Capture->FOVAngle = 70.f;
	// The room is dark on purpose; let the projector film it a few stops brighter.
	Capture->PostProcessSettings.bOverride_AutoExposureBias = true;
	Capture->PostProcessSettings.AutoExposureBias = 3.f;
	Capture->PostProcessBlendWeight = 1.f;
}

void AProjectionScreen::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Basic-shape plane is 100 x 100 in XY facing +Z: stand it up facing +X.
	ScreenMesh->SetRelativeRotation(FRotator::ZeroRotator);
	ScreenMesh->SetRelativeLocation(FVector(4.f, 0.f, 0.f)); // just in front of the frame (frame front is at +2)
	ScreenMesh->SetRelativeScale3D(FVector(0.02f, ScreenSize.X / 100.f, ScreenSize.Y / 100.f));
	Frame->SetRelativeScale3D(FVector(0.04f, (ScreenSize.X + 30.f) / 100.f, (ScreenSize.Y + 30.f) / 100.f));
	Capture->SetWorldLocationAndRotation(CaptureFrom, (CaptureAt - CaptureFrom).Rotation());
}

void AProjectionScreen::BeginPlay()
{
	Super::BeginPlay();
	RetakeVisuals::Tint(Frame, FLinearColor(0.03f, 0.03f, 0.03f));
	Target = NewObject<UTextureRenderTarget2D>(this);
	Target->InitAutoFormat(512, 288);
	Target->ClearColor = FLinearColor::Black;
	Capture->TextureTarget = Target;
	Capture->HiddenComponents.Add(ScreenMesh); // no screen-in-screen feedback
	Caption->SetWorldSize(ScreenSize.Y * 0.16f);
	Capture->SetWorldLocationAndRotation(CaptureFrom, (CaptureAt - CaptureFrom).Rotation());
	ScreenMaterial = ScreenMesh->CreateDynamicMaterialInstance(0);
	if (ScreenMaterial)
	{
		// Fallback (default): a plain lit screen with the dailies caption; live capture only if switched on.
		UTexture* White = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
		ScreenMaterial->SetTextureParameterValue(TEXT("Screen"), bLiveCapture ? static_cast<UTexture*>(Target) : White);
	}
}

void AProjectionScreen::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Clock += DeltaSeconds;
	static constexpr float FrameStep = 1.f / 20.f;
	if (Clock >= FrameStep)
	{
		Clock = FMath::Fmod(Clock, FrameStep);
		if (bLiveCapture)
		{
			Capture->CaptureScene();
		}
		// The dailies: whatever take Jack is on, it's up on the screen.
		if (const ARetakeSceneDirector* D = ARetakeSceneDirector::Get(this))
		{
			const int32 Take = FMath::Max(1, D->GetCurrentTakeNumber());
			Caption->SetText(FText::FromString(FString::Printf(TEXT("DAILIES  -  J. WARD\nTAKE %d"), RetakeScenes::DisplayTake(Take))));
		}
		if (ScreenMaterial)
		{
			// Projector-lamp flicker.
			ScreenMaterial->SetScalarParameterValue(TEXT("Brightness"), Brightness * FMath::FRandRange(0.85f, 1.f));
		}
	}
}

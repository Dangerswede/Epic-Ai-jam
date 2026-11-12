#include "Retake/CameraMark.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Retake/RetakeActivator.h"
#include "Retake/RetakeCharacter.h"
#include "Retake/RetakeSceneDirector.h"
#include "Retake/TakeData.h"

namespace
{
	UStaticMeshComponent* MakePart(AActor* Owner, USceneComponent* Parent, const TCHAR* Name, const TCHAR* Shape)
	{
		UStaticMeshComponent* C = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(Parent);
		C->SetStaticMesh(RetakeVisuals::LoadBasicShape(Shape));
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCanEverAffectNavigation(false);
		C->SetCastShadow(false);
		return C;
	}
}

ACameraMark::ACameraMark()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Ring = MakePart(this, Root, TEXT("Ring"), TEXT("Cylinder"));
	Dot = MakePart(this, Root, TEXT("Dot"), TEXT("Cylinder"));
	CameraStand = MakePart(this, Root, TEXT("CameraStand"), TEXT("Cylinder"));
	CameraBody = MakePart(this, Root, TEXT("CameraBody"), TEXT("Cube"));
	CameraBody->SetCastShadow(true);

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetVerticalAlignment(EVRTA_TextCenter);
	Label->SetWorldSize(34.f);
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACameraMark::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Tape ring on the floor: a thin coloured disc with a darker centre (basic shapes are 100 cm).
	const float D = Radius * 2.f / 100.f;
	Ring->SetRelativeLocation(FVector(0.f, 0.f, 1.f));
	Ring->SetRelativeScale3D(FVector(D, D, 0.02f));
	Dot->SetRelativeLocation(FVector(0.f, 0.f, 2.f));
	Dot->SetRelativeScale3D(FVector(D * 0.82f, D * 0.82f, 0.02f));
	Label->SetRelativeLocation(FVector(0.f, 0.f, 120.f));

	// A camera on a stand at the shot position, looking at the mark.
	const FVector Cam = CameraLocation;
	const float StandHeight = FMath::Max(20.f, Cam.Z - 15.f); // floor (mark height) up to the camera
	CameraStand->SetRelativeLocation(FVector(Cam.X, Cam.Y, StandHeight * 0.5f));
	CameraStand->SetRelativeScale3D(FVector(0.08f, 0.08f, StandHeight / 100.f));
	CameraBody->SetRelativeLocation(Cam);
	CameraBody->SetRelativeRotation((FVector(0.f, 0.f, 60.f) - Cam).Rotation());
	CameraBody->SetRelativeScale3D(FVector(0.55f, 0.28f, 0.32f));
	SetLook(FLinearColor(0.9f, 0.9f, 0.85f), FString::Printf(TEXT("MARK %d"), MarkNumber));
}

void ACameraMark::BeginPlay()
{
	Super::BeginPlay();
	RetakeVisuals::Tint(Dot, FLinearColor(0.05f, 0.05f, 0.06f));
	RetakeVisuals::Tint(CameraStand, FLinearColor(0.08f, 0.08f, 0.08f));
	RetakeVisuals::Tint(CameraBody, FLinearColor(0.12f, 0.12f, 0.13f));
}

bool ACameraMark::IsOnMark(const FVector& Location, float T) const
{
	if (T < WindowStart || T > WindowEnd)
	{
		return false;
	}
	const FVector Delta = Location - GetActorLocation();
	return Delta.Size2D() <= Radius && FMath::Abs(Delta.Z) < 250.f;
}

float ACameraMark::FindHitTime(const FTakeData& Take) const
{
	for (const FTakeFrame& F : Take.Frames)
	{
		if (IsOnMark(F.Location, F.Time))
		{
			return F.Time;
		}
	}
	return -1.f;
}

void ACameraMark::SetLook(const FLinearColor& Color, const FString& Text)
{
	RetakeVisuals::Tint(Ring, Color);
	Label->SetText(FText::FromString(Text));
	Label->SetTextRenderColor(Color.ToFColor(true));
}

void ACameraMark::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ARetakeSceneDirector* D = ARetakeSceneDirector::Get(this);
	if (!D)
	{
		return;
	}
	const ERetakeSceneState S = D->GetState();
	// Marks are crew tape: hidden in the finished film.
	SetActorHiddenInGame(S == ERetakeSceneState::DirectorsCut || S == ERetakeSceneState::Results);
	if (IsHidden())
	{
		return;
	}

	// Label faces the player's camera.
	if (const APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		FRotator R = (Cam->GetCameraLocation() - Label->GetComponentLocation()).Rotation();
		Label->SetWorldRotation(FRotator(0.f, R.Yaw, 0.f));
	}

	const int32 Take = D->GetCurrentTakeNumber();
	if (Take != LastTakeNumber)
	{
		LastTakeNumber = Take;
		bHitThisTake = false;
	}
	bool bInCan = false;
	for (const FTakeData& T : D->GetTakes())
	{
		bInCan |= IsHitBy(T);
	}

	const bool bRolling = S == ERetakeSceneState::Rolling;
	const float Now = bRolling ? D->TakeDurationSeconds - D->GetTakeTimeRemaining() : 0.f;
	if (bRolling && !bHitThisTake)
	{
		const ARetakeCharacter* P = Cast<ARetakeCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
		if (P && IsOnMark(P->GetActorLocation(), Now))
		{
			bHitThisTake = true;
			D->NotifyMarkHit(this);
		}
	}

	const FString Window = FString::Printf(TEXT("%.0f-%.0fs"), WindowStart, WindowEnd);
	if (bInCan || bHitThisTake)
	{
		SetLook(FLinearColor(1.f, 0.72f, 0.05f), FString::Printf(TEXT("MARK %d  IN THE CAN"), MarkNumber));
	}
	else if (bRolling && Now >= WindowStart && Now <= WindowEnd)
	{
		Pulse += DeltaSeconds * 8.f;
		const float A = 0.65f + 0.35f * FMath::Sin(Pulse);
		SetLook(FLinearColor(0.1f, 1.f, 0.25f) * A, FString::Printf(TEXT("MARK %d  NOW! %.1f"), MarkNumber, WindowEnd - Now));
	}
	else if (bRolling && Now > WindowEnd)
	{
		SetLook(FLinearColor(0.25f, 0.25f, 0.25f), FString::Printf(TEXT("MARK %d  missed"), MarkNumber));
	}
	else
	{
		SetLook(FLinearColor(0.9f, 0.9f, 0.85f), FString::Printf(TEXT("MARK %d  %s"), MarkNumber, *Window));
	}
}

#include "Retake/RetakeActivator.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

void ARetakeActivator::SetActive(bool bNewActive)
{
	if (bActive == bNewActive)
	{
		return;
	}
	bActive = bNewActive;
	OnActiveChanged();
	if (bActive)
	{
		OnActivated.Broadcast(this);
	}
	else
	{
		OnDeactivated.Broadcast(this);
	}
}

void RetakeVisuals::Tint(UStaticMeshComponent* Mesh, const FLinearColor& Color)
{
	if (!Mesh || Mesh->GetNumMaterials() == 0)
	{
		return;
	}
	if (UMaterialInstanceDynamic* MID = Mesh->CreateDynamicMaterialInstance(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

UStaticMesh* RetakeVisuals::LoadBasicShape(const TCHAR* Name)
{
	// Only call from constructors.
	const FString Path = FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Name, Name);
	ConstructorHelpers::FObjectFinder<UStaticMesh> Finder(*Path);
	return Finder.Succeeded() ? Finder.Object : nullptr;
}

void RetakeVisuals::ApplyArt(UStaticMeshComponent* Graybox, UStaticMeshComponent* ArtComp, UStaticMesh* ArtAsset, const FVector& TargetSize)
{
	if (!Graybox || !ArtComp)
	{
		return;
	}
	if (!ArtAsset)
	{
		ArtComp->SetStaticMesh(nullptr);
		Graybox->SetVisibility(true);
		return;
	}
	ArtComp->SetStaticMesh(ArtAsset);
	const FVector ArtSize = ArtAsset->GetBoundingBox().GetSize();
	ArtComp->SetUsingAbsoluteScale(true);
	ArtComp->SetWorldScale3D(FVector(
		TargetSize.X / FMath::Max(1.f, ArtSize.X),
		TargetSize.Y / FMath::Max(1.f, ArtSize.Y),
		TargetSize.Z / FMath::Max(1.f, ArtSize.Z)));
	// Stand on the graybox's bottom, centred on it.
	Graybox->UpdateBounds();
	const FBoxSphereBounds B = Graybox->Bounds;
	ArtComp->SetWorldLocation(FVector(B.Origin.X, B.Origin.Y, B.Origin.Z - B.BoxExtent.Z));
	Graybox->SetVisibility(false); // still collides
}

UStaticMeshComponent* RetakeVisuals::CreateArtComponent(AActor* Owner, USceneComponent* Parent)
{
	UStaticMeshComponent* Art = Owner->CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArtMesh"));
	Art->SetupAttachment(Parent);
	Art->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Art->SetGenerateOverlapEvents(false);
	Art->SetMobility(EComponentMobility::Movable);
	Art->SetCanEverAffectNavigation(false);
	return Art;
}

void RetakeVisuals::SpringStep(float& X, float& V, float Target, float DeltaTime, float Stiffness, float Damping)
{
	float Left = FMath::Min(DeltaTime, 0.1f);
	while (Left > 0.f)
	{
		const float H = FMath::Min(Left, 1.f / 120.f);
		V += (Stiffness * (Target - X) - Damping * V) * H;
		X += V * H;
		Left -= H;
	}
}

bool RetakeVisuals::IsPlayerNear(const AActor* Self, float Radius)
{
	if (!Self)
	{
		return false;
	}
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(Self, 0);
	if (!Pawn || Pawn->IsHidden() || !Pawn->GetActorEnableCollision())
	{
		return false; // e.g. the Director's Cut hides the player
	}
	return FVector::DistSquared(Pawn->GetActorLocation(), Self->GetActorLocation()) <= FMath::Square(Radius);
}

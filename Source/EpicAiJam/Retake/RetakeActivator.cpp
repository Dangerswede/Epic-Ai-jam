#include "Retake/RetakeActivator.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
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
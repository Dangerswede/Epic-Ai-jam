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

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Retake/RetakeResettable.h"
#include "RetakeActivator.generated.h"

class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRetakeActivatorSignature, ARetakeActivator*, Activator);

/**
 * Base for things that can be "on" or "off" and drive gates (pressure plates, levers).
 * (Godot: a base script with an `active` property and activated/deactivated signals.)
 */
UCLASS(Abstract)
class EPICAIJAM_API ARetakeActivator : public AActor, public IRetakeResettable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Retake")
	bool IsActive() const { return bActive; }

	UPROPERTY(BlueprintAssignable, Category = "Retake")
	FRetakeActivatorSignature OnActivated;

	UPROPERTY(BlueprintAssignable, Category = "Retake")
	FRetakeActivatorSignature OnDeactivated;

	virtual void ResetForTake() override {}

	/** True if any activator in the list is active. */
	static bool AnyActive(const TArray<TObjectPtr<ARetakeActivator>>& Activators)
	{
		for (const ARetakeActivator* A : Activators)
		{
			if (A && A->IsActive())
			{
				return true;
			}
		}
		return false;
	}

protected:
	/** Sets the state and broadcasts if it changed. */
	void SetActive(bool bNewActive);

	/** Visual hook for subclasses. */
	virtual void OnActiveChanged() {}

private:
	bool bActive = false;
};

namespace RetakeVisuals
{
	/** Tints an engine BasicShape mesh (BasicShapeMaterial has a "Color" parameter). */
	EPICAIJAM_API void Tint(UStaticMeshComponent* Mesh, const FLinearColor& Color);

	/** Loads an engine basic shape: Cube, Cylinder, Sphere, Plane, Cone. */
	EPICAIJAM_API class UStaticMesh* LoadBasicShape(const TCHAR* Name);

	/**
	 * Art pass: show ArtAsset (pivot bottom-centre, any size) in ArtComp, scaled to TargetSize (world cm)
	 * and standing where Graybox's bottom is; the graybox stays as the invisible collision shape.
	 * With no ArtAsset the graybox is shown again.
	 */
	EPICAIJAM_API void ApplyArt(UStaticMeshComponent* Graybox, UStaticMeshComponent* ArtComp, UStaticMesh* ArtAsset, const FVector& TargetSize);

	/** Creates the (collision-free) art mesh component; call from a constructor. */
	EPICAIJAM_API UStaticMeshComponent* CreateArtComponent(AActor* Owner, USceneComponent* Parent);
}

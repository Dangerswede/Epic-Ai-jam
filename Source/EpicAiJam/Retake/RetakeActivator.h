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
}

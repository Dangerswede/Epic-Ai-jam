#pragma once

#include "CoreMinimal.h"
#include "Retake/RetakeActivator.h"
#include "Retake/RetakeInteractable.h"
#include "Lever.generated.h"

class UStaticMeshComponent;

/**
 * Toggles on the Interact action (player or ghost) within range.
 * (Godot: a StaticBody3D with an interact() method that flips a bool and emits a signal.)
 */
UCLASS()
class EPICAIJAM_API ALever : public ARetakeActivator, public IRetakeInteractable
{
	GENERATED_BODY()

public:
	ALever();

	virtual void RetakeInteract(AActor* Interactor) override;
	virtual void ResetForTake() override;

	/** State at the start of every take. */
	UPROPERTY(EditAnywhere, Category = "Retake")
	bool bStartsActive = false;

protected:
	virtual void BeginPlay() override;
	virtual void OnActiveChanged() override;

	/** Swap for art later. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<UStaticMeshComponent> HandleMesh;

	UPROPERTY(EditAnywhere, Category = "Retake")
	float HandleAngle = 35.f;

	UPROPERTY(EditAnywhere, Category = "Retake")
	FLinearColor OffColor = FLinearColor(0.6f, 0.1f, 0.1f);

	UPROPERTY(EditAnywhere, Category = "Retake")
	FLinearColor OnColor = FLinearColor(0.1f, 0.8f, 0.2f);
};

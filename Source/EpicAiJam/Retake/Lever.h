#pragma once

#include "CoreMinimal.h"
#include "Retake/RetakeActivator.h"
#include "Retake/RetakeInteractable.h"
#include "Lever.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

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
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Art pass: optional prop mesh shown instead of the graybox (which stays as the collision). Pivot bottom-centre. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art")
	TObjectPtr<UStaticMesh> ArtMeshAsset;

	/** Size (cm) the art mesh is scaled to. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art")
	FVector ArtSize = FVector(60.f, 46.f, 115.f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Art")
	TObjectPtr<UStaticMeshComponent> ArtMesh;


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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<UTextRenderComponent> UseLabel;

	UPROPERTY(EditAnywhere, Category = "Retake")
	float HandleAngle = 35.f;

	UPROPERTY(EditAnywhere, Category = "Retake")
	FLinearColor OffColor = FLinearColor(0.6f, 0.1f, 0.1f);

	UPROPERTY(EditAnywhere, Category = "Retake")
	FLinearColor OnColor = FLinearColor(0.1f, 0.8f, 0.2f);
};

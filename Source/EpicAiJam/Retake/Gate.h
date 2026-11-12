#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Retake/RetakeResettable.h"
#include "Gate.generated.h"

class ARetakeActivator;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGateSignature, AGate*, Gate);

/**
 * Opens (slides up) while any linked plate or lever is active.
 * (Godot: an AnimatableBody3D that tweens up when any connected switch emits "activated".)
 */
UCLASS()
class EPICAIJAM_API AGate : public AActor, public IRetakeResettable
{
	GENERATED_BODY()

public:
	AGate();

	virtual void Tick(float DeltaSeconds) override;
	virtual void ResetForTake() override;

	/** Plates / levers that open this gate (any one active = open). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retake")
	TArray<TObjectPtr<ARetakeActivator>> LinkedActivators;

	/** If true, ALL linked activators must be active (e.g. two plates); otherwise any one opens the gate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retake")
	bool bRequireAllActivators = false;

	UFUNCTION(BlueprintPure, Category = "Retake")
	bool ShouldBeOpen() const;

	UFUNCTION(BlueprintPure, Category = "Retake")
	float GetOpenAlpha() const { return OpenAlpha; }

	/** Fired when the gate starts opening / closing (for audio). */
	UPROPERTY(BlueprintAssignable, Category = "Retake") FGateSignature OnOpening;
	UPROPERTY(BlueprintAssignable, Category = "Retake") FGateSignature OnClosing;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<USceneComponent> Root;

	/** Swap for art later. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	/** How far the door moves when fully open (local space). */
	UPROPERTY(EditAnywhere, Category = "Retake")
	FVector OpenOffset = FVector(0.f, 0.f, 260.f);

	/** Open/close time in seconds. */
	UPROPERTY(EditAnywhere, Category = "Retake", meta = (ClampMin = "0.01"))
	float MoveTime = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Retake")
	FLinearColor DoorColor = FLinearColor(0.9f, 0.6f, 0.1f);

private:
	void ApplyAlpha();

	FVector ClosedLocation = FVector::ZeroVector;
	float OpenAlpha = 0.f;
	bool bWantsOpen = false;
};

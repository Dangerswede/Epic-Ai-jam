#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Retake/RetakeResettable.h"
#include "LaserBeam.generated.h"

class ARetakeActivator;
class UBoxComponent;
class UStaticMeshComponent;

/**
 * Security laser along local +X. Cuts the take when the player touches it,
 * unless any linked lever/plate is active (then it's switched off). Ghosts pass through.
 * (Godot: an Area3D whose monitoring is toggled by a switch's signal.)
 */
UCLASS()
class EPICAIJAM_API ALaserBeam : public AActor, public IRetakeResettable
{
	GENERATED_BODY()

public:
	ALaserBeam();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void ResetForTake() override;

	/** Any of these active = laser off. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retake")
	TArray<TObjectPtr<ARetakeActivator>> LinkedActivators;

	/** Beam length in cm along local X. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retake", meta = (ClampMin = "10.0"))
	float BeamLength = 400.f;

	UFUNCTION(BlueprintPure, Category = "Retake")
	bool IsBeamOn() const { return bBeamOn; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<UBoxComponent> Trigger;

	/** Swap for art later. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<UStaticMeshComponent> BeamMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<UStaticMeshComponent> EmitterA;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<UStaticMeshComponent> EmitterB;

private:
	void UpdateShape();
	void SetBeamOn(bool bOn);

	bool bBeamOn = true;
};

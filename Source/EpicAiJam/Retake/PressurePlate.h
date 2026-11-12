#pragma once

#include "CoreMinimal.h"
#include "Retake/RetakeActivator.h"
#include "PressurePlate.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 * Active while any character (player or ghost) or weighted actor stands on it.
 * (Godot: an Area3D that is "on" while get_overlapping_bodies() isn't empty.)
 */
UCLASS()
class EPICAIJAM_API APressurePlate : public ARetakeActivator
{
	GENERATED_BODY()

public:
	APressurePlate();

	virtual void ResetForTake() override;

	/** Actors with this tag also press the plate (e.g. crates). Characters always do. */
	UPROPERTY(EditAnywhere, Category = "Retake")
	FName WeightTag = TEXT("RetakeWeight");

protected:
	virtual void BeginPlay() override;
	virtual void OnActiveChanged() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<UBoxComponent> Trigger;

	/** Swap for art later. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<UStaticMeshComponent> PlateMesh;

	UPROPERTY(EditAnywhere, Category = "Retake")
	FLinearColor IdleColor = FLinearColor(0.6f, 0.1f, 0.1f);

	UPROPERTY(EditAnywhere, Category = "Retake")
	FLinearColor PressedColor = FLinearColor(0.1f, 0.8f, 0.2f);

	UPROPERTY(EditAnywhere, Category = "Retake")
	float PressedDepth = 4.f;

private:
	UFUNCTION()
	void HandleOverlapChanged(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	bool CountsAsWeight(const AActor* Actor) const;
	void Reevaluate();

	FVector PlateRestLocation = FVector::ZeroVector;
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Retake/RetakeResettable.h"
#include "GoalZone.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 * The scene's "mark". When the player enters it while Rolling, the director wraps the scene.
 * Ghosts entering it are ignored by the director.
 * (Godot: an Area3D whose body_entered calls the level manager.)
 */
UCLASS()
class EPICAIJAM_API AGoalZone : public AActor, public IRetakeResettable
{
	GENERATED_BODY()

public:
	AGoalZone();

	virtual void ResetForTake() override {}

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<UBoxComponent> Trigger;

	/** Floor marker; swap for art later. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<UStaticMeshComponent> MarkerMesh;

	UPROPERTY(EditAnywhere, Category = "Retake")
	FLinearColor MarkerColor = FLinearColor(0.1f, 0.5f, 1.f);

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);
};

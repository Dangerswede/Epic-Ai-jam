#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Retake/RetakeResettable.h"
#include "KillVolume.generated.h"

class UBoxComponent;

/**
 * Cuts the take when the player enters it (falls off the set, hits a hazard). Ghosts are ignored.
 * (Godot: an Area3D that calls the level manager's "player died".)
 */
UCLASS()
class EPICAIJAM_API AKillVolume : public AActor, public IRetakeResettable
{
	GENERATED_BODY()

public:
	AKillVolume();

	virtual void ResetForTake() override {}

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<UBoxComponent> Trigger;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);
};

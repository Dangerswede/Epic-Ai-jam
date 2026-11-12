#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Retake/RetakeResettable.h"
#include "PushCrate.generated.h"

class UStaticMeshComponent;

/**
 * Crate moved only by kinematic sweeps (no physics simulation), so pushes replay deterministically.
 * - Player walking into a face: crate slides along that axis at PushSpeed, player records a Push action.
 * - Ghost: when its recorded frame has Push, the crate is placed in front of the ghost's recorded position.
 * - Unsupported crates fall straight down (constant speed) - push one into a gap to bridge it.
 * (Godot: an AnimatableBody3D moved with move_and_collide instead of a RigidBody3D.)
 */
UCLASS()
class EPICAIJAM_API APushCrate : public AActor, public IRetakeResettable
{
	GENERATED_BODY()

public:
	APushCrate();

	virtual void Tick(float DeltaSeconds) override;
	virtual void ResetForTake() override;

	/** Edge length of the cube in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retake")
	float Size = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retake")
	float PushSpeed = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retake")
	float FallSpeed = 900.f;

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

	/** Swap for art later. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<UStaticMeshComponent> CrateMesh;

private:
	/** Returns true and the push direction (unit, world, axis-aligned to the crate) if Pusher is touching a face. */
	bool GetContactFace(const AActor* Pusher, FVector& OutDir, float& OutAxisDist) const;
	void TryPushBy(AActor* Pusher, float DeltaSeconds);
	void ApplyFall(float DeltaSeconds);

	FTransform StartTransform;
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Retake/RetakeResettable.h"
#include "Retake/RetakeWorldReplayable.h"
#include "PushCrate.generated.h"

class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPushCrateSignature, APushCrate*, Crate);

/**
 * Crate moved only by kinematic sweeps (no physics simulation), so pushes replay deterministically.
 * - Player walking into a face: crate slides along that axis at PushSpeed, player records a Push action.
 * - Ghost: when its recorded frame has Push, the crate is placed in front of the ghost's recorded position.
 * - A crate falls straight down (constant speed) once no part of its bottom is supported - push it fully over a gap to bridge it.
 * (Godot: an AnimatableBody3D moved with move_and_collide instead of a RigidBody3D.)
 */
UCLASS()
class EPICAIJAM_API APushCrate : public AActor, public IRetakeResettable, public IRetakeWorldReplayable
{
	GENERATED_BODY()

public:
	APushCrate();

	virtual void Tick(float DeltaSeconds) override;
	virtual void ResetForTake() override;
	virtual void SetReplayMode(bool bReplay) override { bReplayMode = bReplay; }

	/** Crate size in cm (X length, Y width, Z height), in the crate's local space. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retake")
	FVector Size = FVector(120.f, 120.f, 120.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retake")
	float PushSpeed = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retake")
	float FallSpeed = 900.f;

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Art pass: optional prop mesh shown instead of the graybox (which stays as the collision). Pivot bottom-centre. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art")
	TObjectPtr<UStaticMesh> ArtMeshAsset;

	/** Size (cm) the art mesh is scaled to; zero = the crate's Size. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art")
	FVector ArtSize = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Art")
	TObjectPtr<UStaticMeshComponent> ArtMesh;

	/** Fired when the crate starts moving after standing still (for audio). */
	UPROPERTY(BlueprintAssignable, Category = "Retake")
	FPushCrateSignature OnPushStarted;

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
	float StillTime = 1.f;
	bool bReplayMode = false;
};

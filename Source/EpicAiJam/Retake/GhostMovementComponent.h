#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GhostMovementComponent.generated.h"

/**
 * Movement for replaying takes: the recording moves the ghost, this component only reports the recorded
 * velocity AND a matching acceleration, so the mannequin's anim blueprint (ShouldMove = speed > 3 and
 * acceleration != 0) plays walk/run/jump instead of idle. It never simulates or brakes on its own.
 * (Godot: a CharacterBody3D you never call move_and_slide() on - you just set velocity for the AnimationTree.)
 */
UCLASS()
class EPICAIJAM_API UGhostMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	/** Called every replay frame by ATakeGhost. */
	void SetReplayKinematics(const FVector& InVelocity)
	{
		Velocity = InVelocity;
		const FVector Planar = FVector(InVelocity.X, InVelocity.Y, 0.f);
		Acceleration = Planar.SizeSquared() > 1.f ? Planar.GetSafeNormal() * GetMaxAcceleration() : FVector::ZeroVector;
	}

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override
	{
		// Driven entirely by the recording: no gravity, no braking, no sweeps.
		UActorComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);
	}
};

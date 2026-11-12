#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Retake/RetakeResettable.h"
#include "StuntGuard.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EStuntGuardState : uint8
{
	Patrol,
	Chase,
	Stuck,   // caught a ghost, stays on it for a while
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FStuntGuardSignature, AStuntGuard*, Guard, AActor*, Target);

/**
 * Security guard with plain C++ logic (no behavior tree):
 * patrols between two points, chases the nearest player/ghost in its sight cone,
 * catching the player cuts the take; catching a ghost keeps the guard stuck on it.
 * (Godot: a CharacterBody3D with a small match-statement state machine in _physics_process.)
 */
UCLASS()
class EPICAIJAM_API AStuntGuard : public ACharacter, public IRetakeResettable
{
	GENERATED_BODY()

public:
	AStuntGuard();

	virtual void Tick(float DeltaSeconds) override;
	virtual void ResetForTake() override;

	UFUNCTION(BlueprintPure, Category = "Retake")
	EStuntGuardState GetGuardState() const { return GuardState; }

	/** Fired when the guard starts chasing someone / catches someone (for audio). */
	UPROPERTY(BlueprintAssignable, Category = "Retake") FStuntGuardSignature OnSpotted;
	UPROPERTY(BlueprintAssignable, Category = "Retake") FStuntGuardSignature OnCaughtSomeone;

	/** Second patrol point, relative to where the guard is placed (drag the diamond in the viewport). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guard", meta = (MakeEditWidget = "true"))
	FVector PatrolPointB = FVector(600.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guard")
	float SightRange = 800.f;

	/** Half angle of the sight cone in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guard")
	float SightHalfAngle = 50.f;

	/** Stops chasing when the target is further than SightRange * this. */
	UPROPERTY(EditAnywhere, Category = "Guard")
	float LoseSightMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Guard")
	float PatrolSpeed = 200.f;

	UPROPERTY(EditAnywhere, Category = "Guard")
	float ChaseSpeed = 380.f;

	UPROPERTY(EditAnywhere, Category = "Guard")
	float CatchDistance = 110.f;

	/** Seconds the guard stays stuck on a caught ghost. */
	UPROPERTY(EditAnywhere, Category = "Guard")
	float StuckSeconds = 3.f;

	/** After being stuck, ignore that ghost for this long (it can be re-spotted afterwards). */
	UPROPERTY(EditAnywhere, Category = "Guard")
	float GhostIgnoreSeconds = 2.f;

protected:
	virtual void BeginPlay() override;

	/** Placeholder body; swap for art later. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	/** Shows the facing direction. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMeshComponent> VisorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UTextRenderComponent> Label;

private:
	AActor* FindVisibleTarget() const;
	bool CanSee(const AActor* Target) const;
	void MoveToward(const FVector& Location, float Speed);
	void OnCaught(AActor* Target);
	void SetGuardState(EStuntGuardState NewState);

	EStuntGuardState GuardState = EStuntGuardState::Patrol;
	FTransform StartTransform;
	bool bHeadingToB = true;

	TWeakObjectPtr<AActor> Target;
	float StuckTimeLeft = 0.f;
	TWeakObjectPtr<AActor> IgnoredGhost;
	float IgnoreTimeLeft = 0.f;
};

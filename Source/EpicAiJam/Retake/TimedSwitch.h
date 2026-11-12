#pragma once

#include "CoreMinimal.h"
#include "Retake/RetakeActivator.h"
#include "Retake/RetakeInteractable.h"
#include "TimedSwitch.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Interact (player or ghost) switches it on for ActiveSeconds, then it switches itself off.
 * Pressing it again restarts the timer. Several timed switches on one gate with "require all"
 * create simultaneity windows: the takes have to hit them close together.
 * (Godot: an interactable with a one-shot Timer that emits activated/deactivated.)
 */
UCLASS()
class EPICAIJAM_API ATimedSwitch : public ARetakeActivator, public IRetakeInteractable
{
	GENERATED_BODY()

public:
	ATimedSwitch();

	virtual void Tick(float DeltaSeconds) override;
	virtual void RetakeInteract(AActor* Interactor) override;
	virtual void ResetForTake() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** How long one press keeps it on. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retake", meta = (ClampMin = "0.5"))
	float ActiveSeconds = 5.f;

	/** Short name shown above it (e.g. "LOCK A"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retake")
	FText DisplayName;

	UFUNCTION(BlueprintPure, Category = "Retake")
	float GetTimeLeft() const { return TimeLeft; }

	/** Art pass: optional prop mesh shown instead of the graybox (which stays as the collision). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art")
	TObjectPtr<UStaticMesh> ArtMeshAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art")
	FVector ArtSize = FVector(50.f, 40.f, 120.f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Art")
	TObjectPtr<UStaticMeshComponent> ArtMesh;

protected:
	virtual void BeginPlay() override;
	virtual void OnActiveChanged() override;

	/** Post with a button on top; swap for art later. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<UStaticMeshComponent> PostMesh;

	/** Lights up while active and shrinks as the time runs out. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<UStaticMeshComponent> TimerBar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<UTextRenderComponent> Label;

private:
	void UpdateVisuals();

	float TimeLeft = 0.f;
};

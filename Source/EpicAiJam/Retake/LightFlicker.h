#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LightFlicker.generated.h"

class ULightComponent;
enum class ERetakeCutReason : uint8;

/**
 * Story dressing (scene 11, night): every time a take is CUT, the set lights stutter for a moment.
 * Place one in a level; it finds the director and all lights itself. No gameplay effect.
 * (Godot: a node listening to the director's "take_cut" signal and tweening light energies.)
 */
UCLASS()
class EPICAIJAM_API ALightFlicker : public AActor
{
	GENERATED_BODY()

public:
	ALightFlicker();
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Flicker")
	float FlickerSeconds = 0.9f;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleTakeCut(int32 TakeNumber, ERetakeCutReason Reason);

	void Restore();

	TArray<TWeakObjectPtr<ULightComponent>> Lights;
	TArray<float> BaseIntensity;
	float TimeLeft = 0.f;
	float StepClock = 0.f;
};

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Retake/TakeData.h"
#include "RetakeRecorderComponent.generated.h"

/**
 * Records the owner's transform + discrete actions at a fixed 30 Hz.
 * Uses accumulated DeltaTime so the sample rate is independent of frame rate.
 * (Godot: a child node whose _process accumulates delta and appends to an Array every 1/30 s.)
 */
UCLASS(ClassGroup = (Retake), meta = (BlueprintSpawnableComponent))
class EPICAIJAM_API URetakeRecorderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	static constexpr float SampleRate = 30.f;
	static constexpr float SampleInterval = 1.f / SampleRate;

	URetakeRecorderComponent();

	/** TakeNumber <= 0 = auto-increment. */
	UFUNCTION(BlueprintCallable, Category = "Retake")
	void StartRecording(int32 TakeNumber = -1);

	/** Stops recording and returns the finished take (also kept as LastTake). */
	UFUNCTION(BlueprintCallable, Category = "Retake")
	FTakeData StopRecording();

	UFUNCTION(BlueprintPure, Category = "Retake")
	bool IsRecording() const { return bRecording; }

	UFUNCTION(BlueprintPure, Category = "Retake")
	const FTakeData& GetLastTake() const { return LastTake; }

	/** Flags an action to be written into the next sampled frame. */
	UFUNCTION(BlueprintCallable, Category = "Retake")
	void RecordAction(ERetakeAction Action);

	/**
	 * Debug until ARetakeSceneDirector exists: start recording on BeginPlay and
	 * restart (new take) when the owning character presses Cut. Ignored if the level has an ARetakeSceneDirector.
	 */
	UPROPERTY(EditAnywhere, Category = "Retake|Debug")
	bool bDebugAutoRecord = true;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	void SampleFrame();

	UFUNCTION()
	void HandleOwnerAction(ERetakeAction Action);

	UFUNCTION()
	void HandleOwnerCut();

	FTakeData CurrentTake;
	FTakeData LastTake;

	int32 NextTakeNumber = 1;
	float Accumulator = 0.f;
	uint8 PendingActions = 0;
	bool bRecording = false;
};

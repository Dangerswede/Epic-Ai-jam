#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Retake/TakeData.h"
#include "TakeGhost.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTakeGhostSignature, ATakeGhost*, Ghost);

/**
 * Replays a recorded take. ACharacter subclass so it shares mesh/anim BP and collision with the player,
 * but movement is driven purely by the recorded frames (CharacterMovement does not simulate).
 * (Godot: a CharacterBody3D whose _process reads positions from an Array instead of calling move_and_slide.)
 */
UCLASS()
class EPICAIJAM_API ATakeGhost : public ACharacter
{
	GENERATED_BODY()

public:
	ATakeGhost();

	/** Load the take to replay. Puts the ghost at the take's first frame. */
	UFUNCTION(BlueprintCallable, Category = "Retake")
	void Init(const FTakeData& InTake);

	/** Start playback. StartTime = position in the take (seconds) to start from, normally 0. */
	UFUNCTION(BlueprintCallable, Category = "Retake")
	void Play(float StartTime = 0.f);

	UFUNCTION(BlueprintCallable, Category = "Retake")
	void Stop();

	UFUNCTION(BlueprintPure, Category = "Retake")
	bool IsPlaying() const { return bPlaying; }

	UFUNCTION(BlueprintPure, Category = "Retake")
	bool HasFinished() const { return bFinished; }

	UFUNCTION(BlueprintPure, Category = "Retake")
	float GetPlaybackTime() const { return PlaybackTime; }

	UFUNCTION(BlueprintPure, Category = "Retake")
	int32 GetTakeNumber() const { return Take.TakeNumber; }

	/** True if the recording has the given action flag on the frames around the current playback time. */
	UFUNCTION(BlueprintPure, Category = "Retake")
	bool IsActionActiveNow(ERetakeAction Action) const;

	/** Playback time of the most recent frame (any action) that was fired, or -1. */
	UFUNCTION(BlueprintPure, Category = "Retake")
	float GetLastActionTime() const { return LastActionTime; }

	/** Fired once when the take runs out; the ghost then freezes in its last pose. */
	UPROPERTY(BlueprintAssignable, Category = "Retake")
	FTakeGhostSignature OnFinished;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	/** Placeholder body (same as the player) until art exists. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UTextRenderComponent> TakeLabel;

	/**
	 * Translucent ghost material. Should have a vector parameter named TintParameterName
	 * and a scalar named OpacityParameterName. If null, the meshes keep their own materials
	 * (a dynamic instance is still created so the parameters can be set if present).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UMaterialInterface> GhostMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FName TintParameterName = TEXT("Tint");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FName OpacityParameterName = TEXT("Opacity");

	/** Sepia. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FLinearColor TintColor = FLinearColor(0.70f, 0.55f, 0.35f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Opacity = 0.45f;

	/** Interact reach used when replaying an Interact action (should match the player's). */
	UPROPERTY(EditAnywhere, Category = "Retake")
	float InteractRadius = 175.f;

private:
	void ApplyGhostMaterial();
	void ApplyFrameAt(float Time);
	void FireActionsUpTo(float Time);
	void FaceLabelToCamera();
	void Finish();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> GhostMIDs;

	FTakeData Take;
	float PlaybackTime = 0.f;
	float LastActionTime = -1.f;
	int32 NextActionFrame = 0;
	int32 CurrentFrame = 0;
	bool bPlaying = false;
	bool bFinished = false;
};

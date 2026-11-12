#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DirectorsCutComponent.generated.h"

class ACineCameraActor;
class ARetakeSceneDirector;
class ATakeGhost;
class AExplosiveProp;
class UDirectorsCutOverlayWidget;

/**
 * Plays the final cinematic: all takes replay together while the view cuts between placed
 * cine cameras (tag "DirectorsCutCam"), preferring the camera nearest the ghost that just did
 * something. Letterbox, grain, sepia and title cards. Space skips (via the director).
 * (Godot: a node that switches Camera3D.current between placed cameras on a Timer.)
 */
UCLASS(ClassGroup = (Retake), meta = (BlueprintSpawnableComponent))
class EPICAIJAM_API UDirectorsCutComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDirectorsCutComponent();

	/** Start the cinematic for already-spawned, already-playing ghosts. */
	void Begin(const TArray<ATakeGhost*>& InGhosts, float LongestTakeSeconds);

	/** Stop and restore the normal view (called by the director on finish or skip). */
	void End();

	bool IsRunning() const { return bRunning; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	FName CameraTag = TEXT("DirectorsCutCam");

	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	FName WideCameraTag = TEXT("WideCam");

	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	float MinShotSeconds = 2.5f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	float MaxShotSeconds = 4.f;

	/** A ghost counts as "interesting" for this long after it performed an action. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	float ActionInterestSeconds = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	float TitleCardSeconds = 2.f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	TSubclassOf<UDirectorsCutOverlayWidget> OverlayClass;

	/** Time scale for explosions and the hero's last second. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Pacing")
	float SlowMoDilation = 0.35f;

	/** Time scale when nobody moves and no action is coming up. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Pacing")
	float FastForwardDilation = 3.f;

	/** Time scale while people walk but no action is coming up within 1.5 s. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Pacing")
	float CruiseDilation = 2.2f;

	/** Post-process look applied to every Director's Cut camera. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Look")
	float FilmGrain = 0.45f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Look")
	float Saturation = 0.85f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Look")
	FLinearColor SepiaTint = FLinearColor(1.f, 0.88f, 0.7f);

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Look")
	float Vignette = 0.6f;

private:
	ARetakeSceneDirector* GetDirector() const;
	void GatherCameras();
	void ApplyLook(ACineCameraActor* Camera) const;
	ATakeGhost* PickInterestingGhost() const;
	void CutTo(ACineCameraActor* Camera);
	void ChooseShot();
	ACineCameraActor* NearestCamera(const FVector& Location, bool bAvoidCurrent) const;
	float ComputeTargetDilation() const;

	UFUNCTION()
	void HandleExploded(AExplosiveProp* Prop);

	UPROPERTY(Transient) TArray<TObjectPtr<ACineCameraActor>> Cameras;
	UPROPERTY(Transient) TObjectPtr<ACineCameraActor> WideCamera;
	UPROPERTY(Transient) TObjectPtr<ACineCameraActor> SpawnedFallback;
	UPROPERTY(Transient) TObjectPtr<ACineCameraActor> CurrentCamera;
	UPROPERTY(Transient) TArray<TObjectPtr<ATakeGhost>> Ghosts;
	UPROPERTY(Transient) TObjectPtr<UDirectorsCutOverlayWidget> Overlay;

	TWeakObjectPtr<ATakeGhost> Subject;
	FRandomStream Rng;
	float Elapsed = 0.f;
	float NextCutAt = 0.f;
	float ReplayLength = 0.f;
	bool bEndCardShown = false;
	float RealElapsed = 0.f;
	float EndCardAtReal = 0.f;
	float SlowMoUntilReal = -1.f;
	float CurrentDilation = 1.f;
	bool bRunning = false;
};

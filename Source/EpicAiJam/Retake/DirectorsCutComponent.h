#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DirectorsCutComponent.generated.h"

class ACineCameraActor;
class AExplosiveProp;
class ARetakeSceneDirector;
class ATakeGhost;
class UDirectorsCutOverlayWidget;

/** One highlight in the reel: a window on the shared replay timeline. */
struct FReelMoment
{
	enum class EKind : uint8 { Action, Mark, Death, Goal };
	float Start = 0.f;
	float End = 0.f;
	float Focus = 0.f;            // the moment itself (for slow-mo)
	EKind Kind = EKind::Action;
	int32 GhostIndex = INDEX_NONE;
	int32 MarkIndex = INDEX_NONE; // F1: a camera mark this take hit (shot from that mark's camera)
};

/**
 * Optional Director's Cut (watched from the results screen): a ~10-12 s highlight reel.
 * All takes replay together, but only the moments with something happening are shown (interacts,
 * stunt deaths, explosions, the hero reaching the goal); dead time between them fast-forwards at
 * 20x behind a black frame. A single dynamic cine camera frames the action: chase cam behind the
 * actor, low angle for deaths/explosions, a final wide on the hero; line traces keep the subject visible.
 * (Godot: like an AnimationPlayer jumping between markers, with a Camera3D that follows the active node.)
 */
UCLASS(ClassGroup = (Retake), meta = (BlueprintSpawnableComponent))
class EPICAIJAM_API UDirectorsCutComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDirectorsCutComponent();

	/** Start the reel for already-spawned, already-playing ghosts (the last ghost is the winning take). */
	void Begin(const TArray<ATakeGhost*>& InGhosts, float LongestTakeSeconds);

	/** Stop and restore the normal view (called by the director on finish or skip). */
	void End();

	bool IsRunning() const { return bRunning; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Seconds shown around an action / before a stunt death / before the hero's wrap. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Reel")
	float ActionLead = 0.9f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Reel")
	float ActionTail = 0.9f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Reel")
	float GoalLead = 2.2f;

	/** Target length of the reel in real seconds (lower-priority moments are dropped to fit). */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Reel")
	float MaxReelSeconds = 15.f;

	/** Time scale for dead time between moments (hidden behind a black frame). */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Reel")
	float SkipDilation = 20.f;

	/** Time scale around deaths, explosions and the hero's last second. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Reel")
	float SlowMoDilation = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	float TitleCardSeconds = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut")
	TSubclassOf<UDirectorsCutOverlayWidget> OverlayClass;

	/** Post-process look of the reel camera. */
	UPROPERTY(EditAnywhere, Category = "Directors Cut|Look")
	float FilmGrain = 0.45f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Look")
	float Saturation = 0.85f;

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Look")
	FLinearColor SepiaTint = FLinearColor(1.f, 0.88f, 0.7f);

	UPROPERTY(EditAnywhere, Category = "Directors Cut|Look")
	float Vignette = 0.6f;

	/** Real seconds the last reel took (for tests). */
	float GetLastReelRealSeconds() const { return RealElapsed; }

	/** F1: how many shots in the last reel came from camera-mark cameras (for tests). */
	int32 GetMarkShotsUsed() const { return MarkShotsUsed; }

private:
	ARetakeSceneDirector* GetDirector() const;
	void BuildMoments();
	void ApplyLook(ACineCameraActor* Camera) const;
	const FReelMoment* MomentAt(float Time) const;
	void UpdateCamera(const FReelMoment* Moment, float RealDelta, bool bSnap);
	FVector ResolveCameraLocation(const FVector& Desired, const FVector& Target) const;
	const class ACameraMark* PickMarkCamera(const FReelMoment& Moment, const FVector& LookAt) const;

	UFUNCTION()
	void HandleExploded(AExplosiveProp* Prop);

	UPROPERTY(Transient) TObjectPtr<ACineCameraActor> ReelCamera;
	UPROPERTY(Transient) TArray<TObjectPtr<ATakeGhost>> Ghosts;
	UPROPERTY(Transient) TObjectPtr<UDirectorsCutOverlayWidget> Overlay;

	TArray<FReelMoment> Moments;
	int32 CurrentMoment = INDEX_NONE;
	FVector ExplosionAt = FVector::ZeroVector;
	FVector LastShotOffset = FVector(NAN);
	float ExplosionRealTime = -100.f;
	float Elapsed = 0.f;        // replay (game) time
	float RealElapsed = 0.f;
	float ReplayLength = 0.f;
	float EndCardAtReal = 0.f;
	float CurrentDilation = 1.f;
	bool bEndCardShown = false;
	bool bRunning = false;
	int32 MarkShotsUsed = 0;
	FString CurrentShotLabel;
	TWeakObjectPtr<const class ACameraMark> LastMarkCam;
};

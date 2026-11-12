#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Retake/TakeData.h"
#include "RetakeSceneDirector.generated.h"

class ARetakeCharacter;
class ATakeGhost;
class UDirectorsCutComponent;

UENUM(BlueprintType)
enum class ERetakeSceneState : uint8
{
	None,
	Briefing,
	Rolling,
	Cut,
	SceneFailed,
	Wrap,
	DirectorsCut,
	Results,
};

UENUM(BlueprintType)
enum class ERetakeCutReason : uint8
{
	PlayerCut,  // player pressed R
	Killed,     // kill volume
	TimeUp,     // TakeDurationSeconds ran out
	Caught,     // a guard caught the player
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRetakeStateChanged, ERetakeSceneState, NewState, ERetakeSceneState, OldState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRetakeTakeStarted, int32, TakeNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRetakeTakeCut, int32, TakeNumber, ERetakeCutReason, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRetakeSceneEvent);

/**
 * One per level. Runs the take loop:
 * Briefing -> Rolling -> Cut -> Rolling ... -> Wrap -> DirectorsCut -> Results
 * Backspace scraps the last take; out of film = the last take is scrapped automatically.
 * (Godot: an autoload-like "level manager" node with a state machine and signals.)
 */
UCLASS()
class EPICAIJAM_API ARetakeSceneDirector : public AActor
{
	GENERATED_BODY()

public:
	ARetakeSceneDirector();

	// --- Scene setup (edit in the level) -------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene")
	FText SceneTitle = FText::FromString(TEXT("Scene 1"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene", meta = (MultiLine = "true"))
	FText ShotListText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene", meta = (ClampMin = "1.0"))
	float TakeDurationSeconds = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene", meta = (ClampMin = "1"))
	int32 MaxTakes = 5;

	/** Takes needed for 3 stars. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene", meta = (ClampMin = "1"))
	int32 ParTakes = 2;

	/** Briefing waits for Space ("Press Space to roll") instead of BriefingSeconds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene")
	bool bWaitForRollInput = true;

	/** Where the player starts every take (any actor, e.g. a PlayerStart). If unset, the player's spawn location is used. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene")
	TObjectPtr<AActor> PlayerStart;

	/** Ghost class to spawn (a Blueprint child can set the ghost material). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene")
	TSubclassOf<ATakeGhost> GhostClass;

	UPROPERTY(EditAnywhere, Category = "Timing", meta = (ClampMin = "0.0"))
	float BriefingSeconds = 4.f;

	UPROPERTY(EditAnywhere, Category = "Timing", meta = (ClampMin = "0.0"))
	float CutPauseSeconds = 2.f;

	UPROPERTY(EditAnywhere, Category = "Timing", meta = (ClampMin = "0.0"))
	float SceneFailedSeconds = 3.f;

	/** Pause after scrapping a take before rolling again. */
	UPROPERTY(EditAnywhere, Category = "Timing", meta = (ClampMin = "0.0"))
	float ScrapPauseSeconds = 1.f;

	UPROPERTY(EditAnywhere, Category = "Timing", meta = (ClampMin = "0.0"))
	float WrapSeconds = 2.f;

	/** Director's Cut with camera cuts, letterbox and title cards (UDirectorsCutComponent). If false, a plain replay with the player camera. */
	UPROPERTY(EditAnywhere, Category = "Timing")
	bool bCinematicDirectorsCut = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<UDirectorsCutComponent> DirectorsCut;

	// --- Events (HUD, audio, camera) -----------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Retake")
	FRetakeStateChanged OnStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Retake")
	FRetakeTakeStarted OnTakeStarted;

	UPROPERTY(BlueprintAssignable, Category = "Retake")
	FRetakeTakeCut OnTakeCut;

	UPROPERTY(BlueprintAssignable, Category = "Retake")
	FRetakeSceneEvent OnSceneFailed;

	/** A take was scrapped with Backspace (TakeNumber = the finished take that was thrown away). */
	UPROPERTY(BlueprintAssignable, Category = "Retake")
	FRetakeTakeStarted OnTakeScrapped;

	/** Cut at MaxTakes: the last take was scrapped automatically. */
	UPROPERTY(BlueprintAssignable, Category = "Retake")
	FRetakeSceneEvent OnOutOfFilm;

	UPROPERTY(BlueprintAssignable, Category = "Retake")
	FRetakeSceneEvent OnWrap;

	// --- API ------------------------------------------------------------------

	/** First director in the world (there should be exactly one). */
	static ARetakeSceneDirector* Get(const UObject* WorldContext);

	/** End the current take. Ignored unless Rolling. */
	UFUNCTION(BlueprintCallable, Category = "Retake")
	void CutTake(ERetakeCutReason Reason);

	/** Called by AGoalZone. Wraps the scene if Who is the player and we are Rolling. */
	UFUNCTION(BlueprintCallable, Category = "Retake")
	void NotifyGoalReached(AActor* Who);

	/** Called by AKillVolume for the player. */
	UFUNCTION(BlueprintCallable, Category = "Retake")
	void NotifyPlayerKilled();

	/** Backspace: throw away the current take and the last finished one, then roll that take again. Only while Rolling with at least one take. */
	UFUNCTION(BlueprintCallable, Category = "Retake")
	void ScrapLastTake();

	/** Every take that was rolled to a cut or wrap since the scene (re)started, including scrapped ones. */
	UFUNCTION(BlueprintPure, Category = "Retake")
	int32 GetTotalTakesShot() const { return TotalTakesShot; }

	/** Called by UDirectorsCutComponent when its sequence is done. */
	UFUNCTION(BlueprintCallable, Category = "Retake")
	void FinishDirectorsCut();

	/** Restart the whole scene from Take 1 (e.g. from the Results screen). */
	UFUNCTION(BlueprintCallable, Category = "Retake")
	void RestartScene();

	UFUNCTION(BlueprintPure, Category = "Retake")
	ERetakeSceneState GetState() const { return State; }

	/** Number of the take currently rolling (or the one that was just cut). */
	UFUNCTION(BlueprintPure, Category = "Retake")
	int32 GetCurrentTakeNumber() const { return CurrentTakeNumber; }

	UFUNCTION(BlueprintPure, Category = "Retake")
	float GetTakeTimeRemaining() const;

	/** All finished takes (failed ones + the final winning take after Wrap). */
	UFUNCTION(BlueprintPure, Category = "Retake")
	const TArray<FTakeData>& GetTakes() const { return Takes; }

	UFUNCTION(BlueprintPure, Category = "Retake")
	ARetakeCharacter* GetPlayer() const { return Player; }

	/** Map name without PIE prefix, e.g. "Scene01_Bridge" (used for saves and the scene list). */
	UFUNCTION(BlueprintPure, Category = "Retake")
	FString GetSceneId() const;

	/** True if the last wrap beat the saved best. */
	UFUNCTION(BlueprintPure, Category = "Retake")
	bool WasLastWrapNewBest() const { return bLastWrapNewBest; }

	/** Space during Briefing rolls the first take; during the Director's Cut it skips. */
	UFUNCTION(BlueprintCallable, Category = "Retake")
	void HandleRollOrSkipInput();

	const TArray<TObjectPtr<ATakeGhost>>& GetGhosts() const { return Ghosts; }

	/** Reset every IRetakeResettable in the level. */
	void ResetResettables();

	/** Destroy current ghosts and spawn one per take in TakesToPlay (not started yet). */
	void SpawnGhosts(const TArray<FTakeData>& TakesToPlay);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void SetState(ERetakeSceneState NewState);
	void StartRolling();
	void StartDirectorsCut();
	void EnterResults();
	void DestroyGhosts();
	void RespawnPlayer();
	void SetPlayerControl(bool bEnabled);
	bool FindPlayer();
	void ScheduleState(float Delay, TFunction<void()> Fn);

	UFUNCTION()
	void HandlePlayerCut();

	UPROPERTY(Transient)
	TObjectPtr<ARetakeCharacter> Player;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATakeGhost>> Ghosts;

	TArray<FTakeData> Takes;
	UFUNCTION()
	void HandlePlayerJumpPressed();

	UFUNCTION()
	void HandlePlayerScrap();

	int32 TotalTakesShot = 0;

	bool bLastWrapNewBest = false;
	ERetakeSceneState State = ERetakeSceneState::None;
	int32 CurrentTakeNumber = 0;
	double TakeStartTime = 0.0;
	FTransform StartTransform;

	FTimerHandle TakeTimer;
	FTimerHandle StateTimer;
};

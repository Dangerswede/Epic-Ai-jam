#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Retake/TakeData.h"
#include "RetakeSceneDirector.generated.h"

class ARetakeCharacter;
class ATakeGhost;
class UDirectorsCutComponent;
class ACameraMark;

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

/** What a take is for, shown before and during the take ("TAKE 2 - INSIDE MAN: Hit all three locks..."). */
USTRUCT(BlueprintType)
struct EPICAIJAM_API FTakeRole
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Retake")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Retake", meta = (MultiLine = "true"))
	FText Task;
};

/** F1: the Director's rating (0-100) that gives the stars: 1 for the wrap, 2 at 65+, 3 at 85+. */
struct FDirectorRating
{
	static constexpr int32 WrapPoints = 45;
	static constexpr int32 ParPoints = 20;
	static constexpr int32 NotePoints = 20;
	static constexpr int32 PointsPerMark = 10;
	static constexpr int32 MaxMarkPoints = 20;
	int32 Wrap = 0;
	int32 Par = 0;
	int32 Note = 0;
	int32 Marks = 0;     // points
	int32 MarksHit = 0;
	int32 Total() const { return FMath::Min(100, Wrap + Par + Note + Marks); }
	static int32 StarsFor(int32 Rating, bool bWrapped) { return !bWrapped ? 0 : Rating >= 85 ? 3 : Rating >= 65 ? 2 : 1; }
};

/** One 30 Hz sample of a world actor (guard, crate) for the Director's Cut world track. */
struct FWorldTrackFrame
{
	FTransform Transform;
	uint8 Extra = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRetakeStateChanged, ERetakeSceneState, NewState, ERetakeSceneState, OldState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRetakeTakeStarted, int32, TakeNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRetakeTakeCut, int32, TakeNumber, ERetakeCutReason, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRetakeSceneEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRetakeMarkHit, int32, MarkNumber);

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

	/** Role per take (index 0 = take 1). Empty = no roles in this scene. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene|Roles")
	TArray<FTakeRole> Roles;

	/** Finale: Roles is a cast list - the player decides which take does which job (order planning). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene|Roles")
	bool bRolesAnyOrder = false;

	/** "CAST: GRIP / DECOY / ..." for any-order scenes (empty otherwise). */
	UFUNCTION(BlueprintPure, Category = "Retake")
	FText GetCastLine() const;

	/** Director's note: the extra goal for the 3rd star. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene|Director's Note")
	ERetakeBonus BonusType = ERetakeBonus::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene|Director's Note")
	float BonusParam = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene|Director's Note")
	FText BonusText;

	/** Briefing waits for Space ("Press Space to roll") instead of BriefingSeconds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene")
	bool bWaitForRollInput = true;

	/** F7: location scout - during the briefing you can walk the set (nothing is recorded, guards stand still,
	 *  marks are visible); Space rolls take 1 from the start mark as usual. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene")
	bool bLocationScout = false;

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

	/** Seconds the recorder keeps running after a stunt death (the flight ends up in the take). */
	UPROPERTY(EditAnywhere, Category = "Timing", meta = (ClampMin = "0.0"))
	float StuntTailSeconds = 0.6f;

	/** Global time dilation for the brief hit-stop on a stunt death (1 = off). */
	UPROPERTY(EditAnywhere, Category = "Timing", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float StuntSlowMoDilation = 0.3f;

	/** Pause after scrapping a take before rolling again. */
	UPROPERTY(EditAnywhere, Category = "Timing", meta = (ClampMin = "0.0"))
	float ScrapPauseSeconds = 1.f;

	UPROPERTY(EditAnywhere, Category = "Timing", meta = (ClampMin = "0.0"))
	float WrapSeconds = 2.f;

	/** Director's Cut with camera cuts, letterbox and title cards (UDirectorsCutComponent). If false, a plain replay with the player camera. */
	UPROPERTY(EditAnywhere, Category = "Timing")
	bool bCinematicDirectorsCut = true;

	/** F6: 70s film look on the whole scene (warm grade, grain, vignette) via an unbound post-process volume. */
	UPROPERTY(EditAnywhere, Category = "Look")
	bool bFilmLook = true;

	UPROPERTY(EditAnywhere, Category = "Look")
	float FilmGrain = 0.22f;

	UPROPERTY(EditAnywhere, Category = "Look")
	FLinearColor FilmTint = FLinearColor(1.f, 0.93f, 0.82f);

	UPROPERTY(EditAnywhere, Category = "Look")
	float FilmSaturation = 0.86f;

	UPROPERTY(EditAnywhere, Category = "Look")
	float FilmVignette = 0.5f;

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

	/** The current take was restarted with Q (TakeNumber = the take that rolls again). */
	UPROPERTY(BlueprintAssignable, Category = "Retake")
	FRetakeTakeStarted OnTakeRetaken;

	/** Cut at MaxTakes: the last take was scrapped automatically. */
	UPROPERTY(BlueprintAssignable, Category = "Retake")
	FRetakeSceneEvent OnOutOfFilm;

	UPROPERTY(BlueprintAssignable, Category = "Retake")
	FRetakeSceneEvent OnWrap;

	/** F1: the player stood on a camera mark inside its window this take. */
	UPROPERTY(BlueprintAssignable, Category = "Retake")
	FRetakeMarkHit OnMarkHit;

	/** G1: the player asked the director (H) or ran out of film - full job texts are shown from now on. */
	UPROPERTY(BlueprintAssignable, Category = "Retake")
	FRetakeSceneEvent OnRoleTasksRevealed;

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

	/** Backspace: throw away the current take and the last finished one, then roll that take again (go back one take). Only while Rolling with at least one take. */
	UFUNCTION(BlueprintCallable, Category = "Retake")
	void ScrapLastTake();

	/** Q: throw away only the take being recorded and roll the same take again from the top. Only while Rolling. */
	UFUNCTION(BlueprintCallable, Category = "Retake")
	void RetakeCurrentTake();

	/** Stunt death (explosion, laser, guard): launch the player away from Origin, short slow-mo, record StuntTailSeconds more, then cut. */
	UFUNCTION(BlueprintCallable, Category = "Retake")
	void BeginStuntDeath(ERetakeCutReason Reason, FVector Origin);

	UFUNCTION(BlueprintPure, Category = "Retake")
	bool IsDying() const { return bDying; }

	/** Every take that was rolled to a cut or wrap since the scene (re)started, including scrapped ones. */
	UFUNCTION(BlueprintPure, Category = "Retake")
	int32 GetTotalTakesShot() const { return TotalTakesShot; }

	/** True if the director's note was fulfilled (evaluate after the wrap). */
	UFUNCTION(BlueprintPure, Category = "Retake")
	bool IsBonusMet() const;

	/** "TAKE N - NAME: Task" for a take number, or empty if that take has no role. */
	UFUNCTION(BlueprintPure, Category = "Retake")
	FText GetRoleLine(int32 TakeNumber) const;

	/** Role name for a take number (empty if none). */
	UFUNCTION(BlueprintPure, Category = "Retake")
	FText GetRoleName(int32 TakeNumber) const;

	UFUNCTION(BlueprintPure, Category = "Retake")
	FText GetRoleTask(int32 TakeNumber) const;

	/** G1: from reel 2 the job text is hidden until the player asks the director (H) or runs out of film. */
	UFUNCTION(BlueprintPure, Category = "Retake")
	bool AreRoleTasksHidden() const;

	UFUNCTION(BlueprintCallable, Category = "Retake")
	void RevealRoleTasks();

	/** H / right shoulder. */
	UFUNCTION()
	void AskDirector();

	/** G2: CUT pause is over and the next take waits for Space. */
	UFUNCTION(BlueprintPure, Category = "Retake")
	bool IsAwaitingRoll() const { return bAwaitingRoll; }

	/** Stars for the current result: 1 wrap, 2 at/under par, 3 at/under par + director's note. */
	UFUNCTION(BlueprintPure, Category = "Retake")
	int32 GetStars() const;

	/** F1: rating breakdown for the kept takes (wrap, par, note, camera marks). */
	FDirectorRating GetRating() const;

	/** Camera marks in the level, sorted by MarkNumber. */
	const TArray<TObjectPtr<ACameraMark>>& GetMarks() const { return Marks; }

	/** Called by a mark when the live player hits it. */
	void NotifyMarkHit(ACameraMark* Mark);

	/** Called by UDirectorsCutComponent when its sequence is done. */
	UFUNCTION(BlueprintCallable, Category = "Retake")
	void FinishDirectorsCut();

	/** From the results screen: watch the optional Director's Cut highlight reel. */
	UFUNCTION(BlueprintCallable, Category = "Retake")
	void WatchDirectorsCut();

	/** G4: after scene 1 (first time), at the end of each reel and in the finale the cut plays by itself. */
	UFUNCTION(BlueprintPure, Category = "Retake")
	bool ShouldAutoPlayDirectorsCut() const;

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
	virtual void Tick(float DeltaSeconds) override;

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

	UFUNCTION()
	void HandlePlayerRetake();

	/** Stop recording/ghosts/timer of the rolling take without keeping it. False if not Rolling (or dying). */
	bool AbortRollingTake();

	int32 TotalTakesShot = 0;
	bool bAwaitingRoll = false;
	bool bRoleTasksRevealed = false;
	UPROPERTY(Transient) TArray<TObjectPtr<ACameraMark>> Marks;
	int32 ScrapCount = 0;
	bool bAnyCaught = false;
	bool bAnyDied = false;
	bool bDying = false;
	bool bResultsRecorded = false;

	// World track (P6): guard/crate transforms of the current take, and of the winning take for the Director's Cut.
	void BeginWorldTrack();
	void SampleWorldTrack();
	void ApplyWorldTrack(float Time);
	void SetWorldReplayMode(bool bReplay);
	TMap<TWeakObjectPtr<AActor>, TArray<FWorldTrackFrame>> CurrentTrack;
	TMap<TWeakObjectPtr<AActor>, TArray<FWorldTrackFrame>> WinningTrack;
	float TrackAccumulator = 0.f;
	float ReplayTime = 0.f;
	bool bRecordingTrack = false;
	bool bReplayingTrack = false;
	FTimerHandle DeathTimer;

	bool bLastWrapNewBest = false;
	ERetakeSceneState State = ERetakeSceneState::None;
	int32 CurrentTakeNumber = 0;
	double TakeStartTime = 0.0;
	FTransform StartTransform;

	FTimerHandle TakeTimer;
	FTimerHandle StateTimer;
};

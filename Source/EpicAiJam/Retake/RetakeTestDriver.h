#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RetakeTestDriver.generated.h"

class ARetakeCharacter;
class ARetakeSceneDirector;

/**
 * Automated playtest: drives the real player through scripted steps (move to points, press E/R/Space)
 * in a running game and checks the scene reacts (ghost replay, plates, gates, lasers, explosive, crate,
 * Wrap -> Director's Cut -> Results, save/unlock).
 * Started with the console command:  Retake.AutoTest solution|wrong
 * Prints "RETAKE_TEST <scene> <scenario>: PASS|FAIL ..." and quits the game when run with -RetakeTestQuit.
 * (Godot: like a GUT integration test that plays the scene with simulated input.)
 */
UCLASS(NotPlaceable, Transient)
class EPICAIJAM_API ARetakeTestDriver : public AActor
{
	GENERATED_BODY()

public:
	ARetakeTestDriver();

	void StartScenario(const FString& Scenario);
	virtual void Tick(float DeltaSeconds) override;

private:
	enum class EStepResult : uint8 { Running, Done, Failed };

	struct FStep
	{
		FString Name;
		float Timeout = 30.f;
		TFunction<EStepResult(float)> Fn;
	};

	// Step builders.
	void MoveTo(float X, float Y, float Tolerance = 60.f, float Timeout = 25.f);
	void Wait(float Seconds);
	void WaitState(uint8 State, float Timeout = 30.f);
	void WaitUntil(const FString& Name, TFunction<bool()> Pred, float Timeout = 30.f);
	void Check(const FString& Name, TFunction<bool()> Pred);
	void Do(const FString& Name, TFunction<void()> Action);

	void BuildSolution(const FString& Scene);
	void BuildWrong(const FString& Scene);
	void AddRollAndResultsChecks(const FString& Scene);

	void Finish(bool bPass, const FString& Message);

	template <typename T> T* FindFirst() const;

	ARetakeSceneDirector* Director = nullptr;
	ARetakeCharacter* Player = nullptr;

	TArray<FStep> Steps;
	int32 StepIndex = 0;
	float StepTime = 0.f;
	float TotalTime = 0.f;
	FString SceneName;
	FString ScenarioName;
	bool bRunning = false;
	bool bSawCineCamera = false;
	bool bEverWrapped = false;
	int32 InteractCount = 0;
	FVector WrapCrateLocation = FVector::ZeroVector;
	float DirectorsCutRealSeconds = 0.f;

	UFUNCTION()
	void HandlePlayerInteract() { ++InteractCount; }
};

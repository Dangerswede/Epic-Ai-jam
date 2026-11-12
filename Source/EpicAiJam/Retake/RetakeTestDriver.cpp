#include "Retake/RetakeTestDriver.h"

#include "CineCameraActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Retake/ExplosiveProp.h"
#include "Retake/Gate.h"
#include "Retake/LaserBeam.h"
#include "Retake/Lever.h"
#include "Retake/PressurePlate.h"
#include "Retake/PushCrate.h"
#include "Retake/RetakeCharacter.h"
#include "Retake/RetakeSaveGame.h"
#include "Retake/RetakeScenes.h"
#include "Retake/RetakeSceneDirector.h"
#include "Retake/StuntGuard.h"
#include "Retake/TakeGhost.h"

DEFINE_LOG_CATEGORY_STATIC(LogRetakeTest, Log, All);

ARetakeTestDriver::ARetakeTestDriver()
{
	PrimaryActorTick.bCanEverTick = true;
	// Run before characters so movement input is consumed this frame.
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	PrimaryActorTick.bTickEvenWhenPaused = true;
}

template <typename T>
T* ARetakeTestDriver::FindFirst() const
{
	for (TActorIterator<T> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

// ---------------------------------------------------------------------------- step builders

void ARetakeTestDriver::MoveTo(float X, float Y, float Tolerance, float Timeout)
{
	const FVector2D Target(X, Y);
	Steps.Add({ FString::Printf(TEXT("MoveTo(%.0f, %.0f)"), X, Y), Timeout, [this, Target, Tolerance](float)
	{
		const FVector2D Loc(Player->GetActorLocation());
		const FVector2D Delta = Target - Loc;
		// Entering the goal zone wraps the scene before the exact point is reached.
		if (Delta.Size() <= Tolerance || Director->GetState() == ERetakeSceneState::Wrap)
		{
			return EStepResult::Done;
		}
		if (Director->GetState() != ERetakeSceneState::Rolling)
		{
			UE_LOG(LogRetakeTest, Display, TEXT("  take was cut while moving"));
			return EStepResult::Failed;
		}
		Player->AddMovementInput(FVector(Delta.GetSafeNormal(), 0.f), 1.f);
		return EStepResult::Running;
	} });
}

void ARetakeTestDriver::Wait(float Seconds)
{
	Steps.Add({ FString::Printf(TEXT("Wait(%.1f)"), Seconds), Seconds + 5.f, [this, Seconds](float)
	{
		return StepTime >= Seconds ? EStepResult::Done : EStepResult::Running;
	} });
}

void ARetakeTestDriver::WaitState(uint8 InState, float Timeout)
{
	const ERetakeSceneState Want = static_cast<ERetakeSceneState>(InState);
	Steps.Add({ FString::Printf(TEXT("WaitState(%s)"), *UEnum::GetValueAsString(Want)), Timeout, [this, Want](float)
	{
		return Director->GetState() == Want ? EStepResult::Done : EStepResult::Running;
	} });
}

void ARetakeTestDriver::WaitUntil(const FString& Name, TFunction<bool()> Pred, float Timeout)
{
	Steps.Add({ TEXT("WaitUntil(") + Name + TEXT(")"), Timeout, [Pred](float)
	{
		return Pred() ? EStepResult::Done : EStepResult::Running;
	} });
}

void ARetakeTestDriver::Check(const FString& Name, TFunction<bool()> Pred)
{
	Steps.Add({ TEXT("Check(") + Name + TEXT(")"), 1.f, [Pred](float)
	{
		return Pred() ? EStepResult::Done : EStepResult::Failed;
	} });
}

void ARetakeTestDriver::Do(const FString& Name, TFunction<void()> Action)
{
	Steps.Add({ Name, 1.f, [Action](float)
	{
		Action();
		return EStepResult::Done;
	} });
}

// ---------------------------------------------------------------------------- scenarios

void ARetakeTestDriver::AddRollAndResultsChecks(const FString& Scene)
{
	const uint8 Wrap = (uint8)ERetakeSceneState::Wrap;
	WaitState(Wrap, 10.f);
	WaitState((uint8)ERetakeSceneState::DirectorsCut, 10.f);
	WaitUntil(TEXT("Director's Cut uses a cine camera"), [this]() { return bSawCineCamera; }, 10.f);
	Check(TEXT("all takes replay as ghosts"), [this]()
	{
		return Director->GetGhosts().Num() == Director->GetTakes().Num();
	});
	if (Scene == TEXT("Scene01_Bridge"))
	{
		// Take 1, scrapped take 2, take 2 again (wrap): 2 in the cut, 3 shot.
		Check(TEXT("2 takes in the cut, 3 shot"), [this]()
		{
			return Director->GetTakes().Num() == 2 && Director->GetTotalTakesShot() == 3;
		});
	}
	WaitState((uint8)ERetakeSceneState::Results, 90.f);
	Check(TEXT("save has the scene wrapped and the next scene unlocked"), [this, Scene]()
	{
		const URetakeSaveGame* Save = URetakeSaveGame::LoadOrCreate();
		const int32 Index = RetakeScenes::IndexOf(Scene);
		const bool bNextOk = !RetakeScenes::All().IsValidIndex(Index + 1) || Save->IsUnlocked(Index + 1);
		return Save && Save->IsWrapped(Scene) && bNextOk;
	});
	Do(TEXT("Press P (pause)"), [this]() { Player->TogglePause(); });
	Check(TEXT("game paused"), [this]() { return UGameplayStatics::IsGamePaused(this); });
	Do(TEXT("Press P (resume)"), [this]() { Player->TogglePause(); });
	Check(TEXT("game resumed"), [this]() { return !UGameplayStatics::IsGamePaused(this); });
}

void ARetakeTestDriver::BuildSolution(const FString& Scene)
{
	auto Roll = [this]() { Do(TEXT("Press Space (roll)"), [this]() { Player->StartJump(); }); };
	auto Cut = [this]() { Do(TEXT("Press R (cut)"), [this]() { Player->Cut(); }); };
	auto Interact = [this]() { Do(TEXT("Press E"), [this]() { Player->Interact(); }); };
	auto NextTake = [this](int32 N)
	{
		WaitUntil(FString::Printf(TEXT("take %d rolling"), N), [this, N]()
		{
			return Director->GetState() == ERetakeSceneState::Rolling && Director->GetCurrentTakeNumber() == N;
		}, 10.f);
	};

	WaitState((uint8)ERetakeSceneState::Briefing, 10.f);
	Roll();
	NextTake(1);

	if (Scene == TEXT("Scene01_Bridge"))
	{
		APressurePlate* Plate = FindFirst<APressurePlate>();
		AGate* Gate = FindFirst<AGate>();
		// Take 1: stand on the plate.
		MoveTo(300, 600);
		MoveTo(2400, 900, 40.f);
		Check(TEXT("plate pressed by player"), [Plate]() { return Plate->IsActive(); });
		WaitUntil(TEXT("gate open"), [Gate]() { return Gate->GetOpenAlpha() > 0.95f; }, 3.f);
		Wait(0.5f);
		Cut();
		// P1: Space roll must not record a jump, and E during the CUT pause must do nothing.
		WaitState((uint8)ERetakeSceneState::Cut, 3.f);
		Check(TEXT("take 1 has no Jump action (Space only rolled)"), [this]()
		{
			for (const FTakeFrame& F : Director->GetTakes()[0].Frames)
			{
				if (F.HasAction(ERetakeAction::Jump)) return false;
			}
			return true;
		});
		Do(TEXT("Press E during CUT"), [this]() { InteractCount = 0; Player->Interact(); });
		Check(TEXT("E ignored during CUT"), [this]() { return InteractCount == 0; });
		// P3: shoot a useless take 2, then scrap it from take 3 -> we get a fresh take 2.
		NextTake(2);
		MoveTo(300, -300);
		Cut();
		NextTake(3);
		Check(TEXT("two ghosts in take 3"), [this]() { return Director->GetGhosts().Num() == 2; });
		Do(TEXT("Press Backspace (scrap)"), [this]() { Player->Scrap(); });
		Check(TEXT("scrapped: only take 1 kept"), [this]() { return Director->GetTakes().Num() == 1; });
		// Take 2 (again): the ghost holds the plate; walk the bridge.
		NextTake(2);
		Check(TEXT("one ghost spawned"), [this]() { return Director->GetGhosts().Num() == 1; });
		MoveTo(100, 0);
		MoveTo(1050, 0);
		WaitUntil(TEXT("ghost holds the plate -> gate open"), [Gate]() { return Gate->GetOpenAlpha() > 0.95f; }, 15.f);
		MoveTo(1700, 0);
		MoveTo(2650, 0);
	}
	else if (Scene == TEXT("Scene02_Heist"))
	{
		ALever* Lever = FindFirst<ALever>();
		// Take 1: get spotted by the guard, then run far away (lure).
		AStuntGuard* Guard = FindFirst<AStuntGuard>();
		MoveTo(1200, -600);
		MoveTo(2000, 250);
		WaitUntil(TEXT("guard spots the player"), [Guard]() { return Guard->GetGuardState() == EStuntGuardState::Chase; }, 8.f);
		MoveTo(2300, -1450, 80.f);
		MoveTo(-650, -1450, 80.f); // keep running: the lure only lasts while this take plays (P4)
		WaitUntil(TEXT("guard catches up (or 8 s)"), [this]() { return Director->GetState() != ERetakeSceneState::Rolling; }, 8.f);
		Do(TEXT("Press R (cut) if still rolling"), [this]()
		{
			if (Director->GetState() == ERetakeSceneState::Rolling) Player->Cut();
		});
		// Take 2: while the guard chases ghost 1, pull the lever.
		NextTake(2);
		WaitUntil(TEXT("ghost 1 drags the guard away from the lever"), [this]()
		{
			const AStuntGuard* G = FindFirst<AStuntGuard>();
			const ALever* L = FindFirst<ALever>();
			return G->GetGuardState() != EStuntGuardState::Patrol && FVector::Dist2D(G->GetActorLocation(), L->GetActorLocation()) > 1500.f;
		}, 20.f);
		MoveTo(1400, 960, 40.f);
		Check(TEXT("margin: guard still busy and far from the lever"), [this]()
		{
			const AStuntGuard* G = FindFirst<AStuntGuard>();
			const ALever* L = FindFirst<ALever>();
			return G->GetGuardState() != EStuntGuardState::Patrol && FVector::Dist2D(G->GetActorLocation(), L->GetActorLocation()) > 1200.f;
		});
		Interact();
		Check(TEXT("lever on"), [Lever]() { return Lever->IsActive(); });
		Check(TEXT("lasers off"), [this]()
		{
			for (TActorIterator<ALaserBeam> It(GetWorld()); It; ++It) { if (It->IsBeamOn()) return false; }
			return true;
		});
		MoveTo(600, 960);
		Do(TEXT("Press R (cut) if still rolling"), [this]()
		{
			if (Director->GetState() == ERetakeSceneState::Rolling) Player->Cut();
		});
		// Take 3: wait for ghost 2 to pull the lever, walk into the vault.
		NextTake(3);
		Check(TEXT("two ghosts spawned"), [this]() { return Director->GetGhosts().Num() == 2; });
		// Stay out of sight until ghost 1 has dragged the guard south, then go along the north side.
		WaitUntil(TEXT("ghost 1 drags the guard away"), [this]()
		{
			const AStuntGuard* G = FindFirst<AStuntGuard>();
			const ALever* L = FindFirst<ALever>();
			return G->GetGuardState() != EStuntGuardState::Patrol && FVector::Dist2D(G->GetActorLocation(), L->GetActorLocation()) > 1500.f;
		}, 20.f);
		MoveTo(1500, 350);
		MoveTo(2150, 150);
		WaitUntil(TEXT("ghost 2 pulled the lever -> lasers off"), [this]()
		{
			for (TActorIterator<ALaserBeam> It(GetWorld()); It; ++It) { if (It->IsBeamOn()) return false; }
			return true;
		}, 20.f);
		MoveTo(2250, 0);
		MoveTo(3000, 0);
	}
	else if (Scene == TEXT("Scene03_Chase"))
	{
		AGate* Gate = FindFirst<AGate>();
		AExplosiveProp* Bomb = FindFirst<AExplosiveProp>();
		APushCrate* Crate = FindFirst<APushCrate>();
		AActor* Wall = nullptr;
		for (TActorIterator<AActor> It(GetWorld()); It; ++It) { if (It->ActorHasTag(TEXT("BreakableWall"))) { Wall = *It; break; } }

		// Take 1: hold the plate.
		MoveTo(200, -700, 40.f);
		WaitUntil(TEXT("gate open"), [Gate]() { return Gate->GetOpenAlpha() > 0.95f; }, 3.f);
		Wait(0.5f);
		Cut();
		// Take 2: arm the explosive and get blown up.
		NextTake(2);
		MoveTo(700, 0);
		WaitUntil(TEXT("ghost 1 holds the plate -> gate open"), [Gate]() { return Gate->GetOpenAlpha() > 0.95f; }, 15.f);
		MoveTo(1480, 40, 40.f);
		Interact();
		Check(TEXT("explosive armed"), [Bomb]() { return Bomb->IsArmed(); });
		WaitUntil(TEXT("take 2 cut by the blast"), [this]() { return Director->GetState() == ERetakeSceneState::Cut; }, 5.f);
		Check(TEXT("wall broken"), [Wall]() { return Wall && Wall->IsHidden(); });
		// P5: stunt death - thrown at least 3 m, last frame carries Died.
		Check(TEXT("take 2 is a stunt death (thrown, Died on last frame)"), [this, Bomb]()
		{
			const FTakeData& T = Director->GetTakes()[1];
			const float Thrown = FVector::Dist2D(T.Frames.Last().Location, Bomb->GetActorLocation());
			UE_LOG(LogRetakeTest, Display, TEXT("  thrown to %.0f cm from the bomb"), Thrown);
			return T.EndsInDeath() && T.EndReason == ERetakeCutReason::Killed && Thrown > 300.f;
		});
		// Take 3: wait for ghost 2's blast, then push the crate into the pit and escape.
		NextTake(3);
		Check(TEXT("wall restored for the new take"), [Wall]() { return Wall && !Wall->IsHidden(); });
		WaitUntil(TEXT("ghost 2 armed it -> explosion"), [Bomb]() { return Bomb->HasExploded(); }, 25.f);
		WaitUntil(TEXT("ghost 2 flies the same way and stays down (no collision)"), [this]()
		{
			const ATakeGhost* G = Director->GetGhosts().Num() > 1 ? Director->GetGhosts()[1].Get() : nullptr;
			return G && G->HasFinished() && !G->GetActorEnableCollision() &&
				FVector::Dist2D(G->GetActorLocation(), Director->GetTakes()[1].Frames.Last().Location) < 5.f;
		}, 5.f);
		MoveTo(700, -110);
		MoveTo(1650, -110);
		MoveTo(1940, 0, 30.f);
		MoveTo(2900, 0, 40.f, 15.f);
		WaitUntil(TEXT("crate dropped into the pit"), [Crate]() { return Crate->GetActorLocation().Z < -60.f; }, 8.f);
		MoveTo(4350, 0);
	}
	else if (Scene == TEXT("Scene04_Finale"))
	{
		AGate* Gate = FindFirst<AGate>();
		AExplosiveProp* Bomb = FindFirst<AExplosiveProp>();
		APushCrate* Crate = FindFirst<APushCrate>();
		AStuntGuard* Guard = FindFirst<AStuntGuard>();
		ALever* Lever = FindFirst<ALever>();
		auto LasersOff = [this]()
		{
			for (TActorIterator<ALaserBeam> It(GetWorld()); It; ++It) { if (It->IsBeamOn()) return false; }
			return true;
		};
		auto CutIfRolling = [this]()
		{
			Do(TEXT("Press R (cut) if still rolling"), [this]()
			{
				if (Director->GetState() == ERetakeSceneState::Rolling) Player->Cut();
			});
		};

		// Take 1: hold the plate.
		MoveTo(200, -700, 40.f);
		WaitUntil(TEXT("gate open"), [Gate]() { return Gate->GetOpenAlpha() > 0.95f; }, 3.f);
		Wait(0.5f);
		Cut();
		// Take 2: arm the explosive, get blown up.
		NextTake(2);
		MoveTo(700, 0);
		WaitUntil(TEXT("ghost 1 holds the plate -> gate open"), [Gate]() { return Gate->GetOpenAlpha() > 0.95f; }, 15.f);
		MoveTo(1480, 40, 40.f);
		Interact();
		WaitUntil(TEXT("take 2 cut by the blast"), [this]() { return Director->GetState() == ERetakeSceneState::Cut; }, 5.f);
		// Take 3: after ghost 2's blast, lure the guard away.
		NextTake(3);
		WaitUntil(TEXT("ghost 2's explosion"), [Bomb]() { return Bomb->HasExploded(); }, 25.f);
		MoveTo(700, -110);
		MoveTo(1900, -110);
		MoveTo(2700, -600);
		MoveTo(3500, 250);
		WaitUntil(TEXT("guard spots the player"), [Guard]() { return Guard->GetGuardState() == EStuntGuardState::Chase; }, 8.f);
		MoveTo(3800, -1400, 80.f);
		MoveTo(2150, -1400, 80.f); // keep running west, away from the laser door (P4)
		WaitUntil(TEXT("guard catches up (or 8 s)"), [this]() { return Director->GetState() != ERetakeSceneState::Rolling; }, 8.f);
		CutIfRolling();
		// Take 4: lever, lasers, crate, rooftop.
		NextTake(4);
		Check(TEXT("three ghosts spawned"), [this]() { return Director->GetGhosts().Num() == 3; });
		WaitUntil(TEXT("ghost 2's explosion"), [Bomb]() { return Bomb->HasExploded(); }, 25.f);
		MoveTo(700, -110);
		MoveTo(1900, -110);
		WaitUntil(TEXT("ghost 3 lures the guard"), [Guard]() { return Guard->GetGuardState() != EStuntGuardState::Patrol; }, 15.f);
		Wait(2.f);
		MoveTo(2900, 960, 40.f);
		Check(TEXT("margin: guard still busy and far from the lever"), [Guard, Lever]()
		{
			return Guard->GetGuardState() != EStuntGuardState::Patrol && FVector::Dist2D(Guard->GetActorLocation(), Lever->GetActorLocation()) > 1200.f;
		});
		Interact();
		Check(TEXT("lever on"), [Lever]() { return Lever->IsActive(); });
		Check(TEXT("lasers off"), LasersOff);
		MoveTo(4200, 0);
		MoveTo(4560, 0, 30.f);
		MoveTo(5500, 0, 40.f, 15.f);
		WaitUntil(TEXT("crate dropped into the pit"), [Crate]() { return Crate->GetActorLocation().Z < -60.f; }, 8.f);
		MoveTo(6250, 0);
		MoveTo(7150, 0);
		MoveTo(7600, 0);
	}
	else
	{
		Finish(false, TEXT("no solution script for this scene"));
		return;
	}
	AddRollAndResultsChecks(Scene);
}

void ARetakeTestDriver::BuildWrong(const FString& Scene)
{
	WaitState((uint8)ERetakeSceneState::Briefing, 10.f);
	Do(TEXT("Press Space (roll)"), [this]() { Player->StartJump(); });
	WaitState((uint8)ERetakeSceneState::Rolling, 5.f);

	// One take straight for the goal: must end in a CUT, never a Wrap.
	if (Scene == TEXT("Scene01_Bridge"))
	{
		MoveTo(100, 0);
		MoveTo(1050, 0);
		MoveTo(2650, 0, 60.f, 40.f);
	}
	else if (Scene == TEXT("Scene02_Heist"))
	{
		MoveTo(1900, -250);
		MoveTo(3000, 0, 60.f, 40.f);
	}
	else if (Scene == TEXT("Scene03_Chase"))
	{
		MoveTo(200, -700, 40.f);
		MoveTo(4350, 0, 60.f, 40.f); // gate closes behind the plate
	}
	else if (Scene == TEXT("Scene04_Finale"))
	{
		MoveTo(200, -700, 40.f);
		MoveTo(7600, 0, 60.f, 40.f); // gate closes behind the plate
	}
	// The MoveTo above is expected to time out or the take to be cut; either way: no wrap.
	Steps.Last().Fn = [Inner = Steps.Last().Fn, this](float Dt)
	{
		if (Director->GetState() != ERetakeSceneState::Rolling)
		{
			return EStepResult::Done; // cut (laser/timeout) - fine
		}
		const EStepResult R = Inner(Dt);
		return R == EStepResult::Done ? EStepResult::Failed : R; // reaching the goal area = bad
	};
	Steps.Last().Timeout = 45.f;
	Steps.Last().Name += TEXT(" [must NOT succeed]");
	WaitUntil(TEXT("take ends in a CUT"), [this]()
	{
		return Director->GetState() == ERetakeSceneState::Cut || Director->GetState() == ERetakeSceneState::Rolling;
	}, 40.f);
	Check(TEXT("never wrapped"), [this]() { return !bEverWrapped; });

	if (Scene == TEXT("Scene01_Bridge"))
	{
		// P3 out of film: cut quickly until the last take, cut it too -> it is scrapped, nothing is wiped.
		for (int32 Take = 2; Take <= 5; ++Take)
		{
			WaitUntil(FString::Printf(TEXT("take %d rolling"), Take), [this, Take]()
			{
				return Director->GetState() == ERetakeSceneState::Rolling && Director->GetCurrentTakeNumber() == Take;
			}, 10.f);
			Wait(0.3f);
			Do(TEXT("Press R (cut)"), [this]() { Player->Cut(); });
		}
		Check(TEXT("out of film: take 5 scrapped, 4 kept"), [this]() { return Director->GetTakes().Num() == 4; });
		WaitUntil(TEXT("take 5 rolls again"), [this]()
		{
			return Director->GetState() == ERetakeSceneState::Rolling && Director->GetCurrentTakeNumber() == 5;
		}, 10.f);
	}
}

void ARetakeTestDriver::StartScenario(const FString& Scenario)
{
	Director = ARetakeSceneDirector::Get(this);
	Player = Director ? Director->GetPlayer() : nullptr;
	if (!Player)
	{
		if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			Player = Cast<ARetakeCharacter>(PC->GetPawn());
		}
	}
	SceneName = Director ? Director->GetSceneId() : TEXT("?");
	if (Player)
	{
		Player->OnInteract.AddUniqueDynamic(this, &ARetakeTestDriver::HandlePlayerInteract);
	}
	ScenarioName = Scenario;
	if (!Director || !Player)
	{
		Finish(false, TEXT("no director or player in this map"));
		return;
	}
	Steps.Reset();
	StepIndex = 0;
	StepTime = 0.f;
	bRunning = true;
	if (Scenario.Equals(TEXT("wrong"), ESearchCase::IgnoreCase))
	{
		BuildWrong(SceneName);
	}
	else
	{
		BuildSolution(SceneName);
	}
	UE_LOG(LogRetakeTest, Display, TEXT("RETAKE_TEST %s %s: started, %d steps"), *SceneName, *ScenarioName, Steps.Num());
}

void ARetakeTestDriver::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bRunning)
	{
		return;
	}
	TotalTime += DeltaSeconds;

	if (Director->GetState() == ERetakeSceneState::Wrap)
	{
		bEverWrapped = true;
	}
	if (Director->GetState() == ERetakeSceneState::DirectorsCut)
	{
		if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			bSawCineCamera |= Cast<ACineCameraActor>(PC->GetViewTarget()) != nullptr;
		}
	}

	if (!Steps.IsValidIndex(StepIndex))
	{
		Finish(true, FString::Printf(TEXT("all %d steps passed (%.1f s game time)"), Steps.Num(), TotalTime));
		return;
	}
	FStep& Step = Steps[StepIndex];
	StepTime += DeltaSeconds;
	const EStepResult R = Step.Fn(DeltaSeconds);
	if (R == EStepResult::Done)
	{
		UE_LOG(LogRetakeTest, Display, TEXT("  ok   [%5.1fs] %s"), TotalTime, *Step.Name);
		++StepIndex;
		StepTime = 0.f;
	}
	else if (R == EStepResult::Failed || StepTime > Step.Timeout)
	{
		const FVector L = Player->GetActorLocation();
		Finish(false, FString::Printf(TEXT("step %d '%s' %s (player at %.0f,%.0f,%.0f, state %s, take %d)"),
			StepIndex + 1, *Step.Name, R == EStepResult::Failed ? TEXT("failed") : TEXT("timed out"),
			L.X, L.Y, L.Z, *UEnum::GetValueAsString(Director->GetState()), Director->GetCurrentTakeNumber()));
	}
}

void ARetakeTestDriver::Finish(bool bPass, const FString& Message)
{
	bRunning = false;
	UE_LOG(LogRetakeTest, Display, TEXT("RETAKE_TEST %s %s: %s - %s"), *SceneName, *ScenarioName, bPass ? TEXT("PASS") : TEXT("FAIL"), *Message);
	if (FParse::Param(FCommandLine::Get(), TEXT("RetakeTestQuit")))
	{
		FPlatformMisc::RequestExit(false);
	}
	SetActorTickEnabled(false);
}

// ---------------------------------------------------------------------------- console command

static void RunRetakeAutoTest(const TArray<FString>& Args, UWorld* World)
{
	if (!World)
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ARetakeTestDriver* Driver = World->SpawnActor<ARetakeTestDriver>(Params);
	const FString Scenario = Args.Num() > 0 ? Args[0] : TEXT("solution");
	// Let the director find its player first.
	World->GetTimerManager().SetTimerForNextTick([Driver, Scenario]()
	{
		if (IsValid(Driver))
		{
			Driver->StartScenario(Scenario);
		}
	});
}

static FAutoConsoleCommandWithWorldAndArgs GRetakeAutoTestCmd(
	TEXT("Retake.AutoTest"),
	TEXT("Automated playtest of the current scene: Retake.AutoTest solution|wrong"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RunRetakeAutoTest));

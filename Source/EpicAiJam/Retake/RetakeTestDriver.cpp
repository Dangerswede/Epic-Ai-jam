#include "Retake/RetakeTestDriver.h"

#include "CineCameraActor.h"
#include "Components/TextRenderComponent.h"
#include "Misc/App.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
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
#include "Retake/RetakeHUD.h"
#include "Retake/RetakeHUDWidget.h"
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

APushCrate* ARetakeTestDriver::FindCrate(bool bBiggest) const
{
	APushCrate* Best = nullptr;
	for (TActorIterator<APushCrate> It(GetWorld()); It; ++It)
	{
		if (!Best || (bBiggest ? It->Size.X > Best->Size.X : It->Size.X < Best->Size.X))
		{
			Best = *It;
		}
	}
	return Best;
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
	// P6: remember where the crate ended in the winning take.
	Do(TEXT("remember crate position at wrap"), [this]()
	{
		const APushCrate* Crate = FindCrate(true);
		WrapCrateLocation = Crate ? Crate->GetActorLocation() : FVector::ZeroVector;
	});
	Check(TEXT("no take idles more than 3 s (scenes with roles)"), [this]()
	{
		bool bOk = true;
		for (const TPair<int32, float>& P : MaxIdlePerTake)
		{
			UE_LOG(LogRetakeTest, Display, TEXT("  take %d longest idle: %.1f s"), P.Key, P.Value);
			bOk &= P.Value <= 3.f;
		}
		return bOk || Director->Roles.Num() == 0;
	});
	if (Scene == TEXT("Scene01_Bridge"))
	{
		// Take 1, scrapped take 2, take 2 again (wrap): 2 in the cut, 3 shot.
		Check(TEXT("2 takes in the cut, 3 shot"), [this]()
		{
			return Director->GetTakes().Num() == 2 && Director->GetTotalTakesShot() == 3;
		});
	}
	// R1: no automatic Director's Cut - straight to the results.
	WaitState((uint8)ERetakeSceneState::Results, 10.f);
	Check(TEXT("stars: 1 wrap, 2 par, 3 par + director's note"), [this, Scene]()
	{
		const int32 Stars = Director->GetStars();
		const int32 Expected = 3; // every intended solution meets par and its director's note
		const URetakeSaveGame* Save = URetakeSaveGame::LoadOrCreate();
		const int32* Saved = Save ? Save->BestStars.Find(Scene) : nullptr;
		UE_LOG(LogRetakeTest, Display, TEXT("  stars %d (expected %d), saved %d, note met: %d"), Stars, Expected, Saved ? *Saved : -1, Director->IsBonusMet() ? 1 : 0);
		return Stars == Expected && Saved && *Saved == Stars;
	});
	Check(TEXT("save has the scene wrapped and the next scene unlocked"), [this, Scene]()
	{
		const URetakeSaveGame* Save = URetakeSaveGame::LoadOrCreate();
		const int32 Index = RetakeScenes::IndexOf(Scene);
		const bool bNextOk = !RetakeScenes::All().IsValidIndex(Index + 1) || Save->IsUnlocked(Index + 1);
		return Save && Save->IsWrapped(Scene) && bNextOk;
	});
	// Watch the optional Director's Cut: Space on the results screen (same as the button).
	Wait(0.5f);
	Do(TEXT("Press Space on the results (watch Director's Cut)"), [this]() { DirectorsCutRealSeconds = 0.f; Player->StartJump(); });
	WaitState((uint8)ERetakeSceneState::DirectorsCut, 3.f);
	WaitUntil(TEXT("Director's Cut uses a cine camera"), [this]() { return bSawCineCamera; }, 10.f);
	Check(TEXT("all takes replay as ghosts"), [this]()
	{
		return Director->GetGhosts().Num() == Director->GetTakes().Num();
	});
	Check(TEXT("Director's Cut actors: opaque colours, no TAKE labels"), [this]()
	{
		for (const ATakeGhost* G : Director->GetGhosts())
		{
			if (const UTextRenderComponent* L = G->FindComponentByClass<UTextRenderComponent>()) { if (L->IsVisible()) return false; }
		}
		return true;
	});
	WaitState((uint8)ERetakeSceneState::Results, 60.f);
	if (Scene == TEXT("Scene03_Chase") || Scene == TEXT("Scene04_Finale"))
	{
		Check(TEXT("crate ends exactly where it did in the winning take"), [this]()
		{
			const float D = FVector::Dist(FindCrate(true)->GetActorLocation(), WrapCrateLocation);
			UE_LOG(LogRetakeTest, Display, TEXT("  crate replay error: %.2f cm"), D);
			return D < 1.f;
		});
	}
	Check(TEXT("Director's Cut is a short highlight reel"), [this]()
	{
		UE_LOG(LogRetakeTest, Display, TEXT("  Director's Cut took %.1f s real time"), DirectorsCutRealSeconds);
		return DirectorsCutRealSeconds > 2.f && DirectorsCutRealSeconds < 14.f;
	});
	Check(TEXT("back on the results, save written once"), [this, Scene]()
	{
		const URetakeSaveGame* Save = URetakeSaveGame::LoadOrCreate();
		return Director->GetState() == ERetakeSceneState::Results && Save && Save->IsWrapped(Scene);
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
		// D2: GRIP hits switch A, runs under the bridge, hits switch B; HERO crosses through both windows.
		AGate* GateA = nullptr;
		AGate* GateB = nullptr;
		for (TActorIterator<AGate> It(GetWorld()); It; ++It) { (It->GetActorLocation().X < 1900.f ? GateA : GateB) = *It; }
		// Take 1: GRIP.
		MoveTo(250, -380, 40.f);
		Interact();
		WaitUntil(TEXT("switch A opens gate A"), [GateA]() { return GateA->ShouldBeOpen(); }, 1.f);
		MoveTo(2480, -500, 40.f);
		Interact();
		WaitUntil(TEXT("switch B opens gate B"), [GateB]() { return GateB->ShouldBeOpen(); }, 1.f);
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
		MoveTo(600, 400);
		Cut();
		NextTake(3);
		Check(TEXT("two ghosts in take 3"), [this]() { return Director->GetGhosts().Num() == 2; });
		Do(TEXT("Press Backspace (scrap)"), [this]() { Player->Scrap(); });
		Check(TEXT("scrapped: only take 1 kept"), [this]() { return Director->GetTakes().Num() == 1; });
		// Take 2 (again): HERO.
		NextTake(2);
		Check(TEXT("HUD timeline shows 1 earlier take"), [this]()
		{
			const APlayerController* PC = GetWorld()->GetFirstPlayerController();
			const ARetakeHUD* Hud = PC ? PC->GetHUD<ARetakeHUD>() : nullptr;
			return Hud && Hud->GetHUDWidget() && Hud->GetHUDWidget()->GetTimelineRowCount() == 1;
		});
		Check(TEXT("one ghost spawned"), [this]() { return Director->GetGhosts().Num() == 1; });
		MoveTo(100, 0);
		MoveTo(1050, 0);
		WaitUntil(TEXT("ghost's switch A -> gate A open"), [GateA]() { return GateA->GetOpenAlpha() > 0.95f; }, 5.f);
		MoveTo(2150, 0);
		WaitUntil(TEXT("ghost's switch B -> gate B open"), [GateB]() { return GateB->GetOpenAlpha() > 0.95f; }, 5.f);
		MoveTo(2650, 0);
	}	else if (Scene == TEXT("Scene02_Heist"))
	{
		// D3: DECOY / INSIDE MAN / HERO.
		AStuntGuard* Guard = FindFirst<AStuntGuard>();
		AGate* Vault = FindFirst<AGate>();
		// Take 1: DECOY - get spotted, run the south wall to the east for the whole take.
		MoveTo(500, 350);
		WaitUntil(TEXT("guard spots the decoy"), [Guard]() { return Guard->GetGuardState() == EStuntGuardState::Chase; }, 3.f);
		MoveTo(-600, -1300, 80.f);
		// Keep running until the 12 s take ends (time up).
		Steps.Add({ TEXT("run east until the take ends"), 20.f, [this](float)
		{
			if (Director->GetState() != ERetakeSceneState::Rolling) return EStepResult::Done;
			Player->AddMovementInput(FVector(1.f, 0.f, 0.f), 1.f);
			return EStepResult::Running;
		} });
		// Take 2: INSIDE MAN - three locks along the north wall.
		NextTake(2);
		MoveTo(-300, 900);
		MoveTo(1000, 1180, 40.f);
		Interact();
		MoveTo(1300, 1180, 40.f);
		Interact();
		MoveTo(1600, 1180, 40.f);
		Interact();
		WaitUntil(TEXT("all three locks on -> vault open"), [Vault]() { return Vault->ShouldBeOpen(); }, 1.f);
		Cut();
		// Take 3: HERO - through the vault window.
		NextTake(3);
		Check(TEXT("two ghosts spawned"), [this]() { return Director->GetGhosts().Num() == 2; });
		WaitUntil(TEXT("guard goes after the decoy"), [Guard]() { return Guard->GetGuardState() == EStuntGuardState::Chase; }, 3.f);
		MoveTo(900, -300);
		MoveTo(2300, -100);
		WaitUntil(TEXT("ghost 2's locks open the vault"), [Vault]() { return Vault->GetOpenAlpha() > 0.95f; }, 3.f);
		MoveTo(3000, 0);
	}	else if (Scene == TEXT("Scene03_Chase"))
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
		APushCrate* Crate = FindCrate(true);      // long bridge crate in the alley
		APushCrate* PropCrate = FindCrate(false); // small prop crate in the start room
		AStuntGuard* Guard = FindFirst<AStuntGuard>();
		ALever* Lever = FindFirst<ALever>();
		APressurePlate* PlateN = nullptr;
		APressurePlate* PlateS = nullptr;
		for (TActorIterator<APressurePlate> It(GetWorld()); It; ++It)
		{
			(It->GetActorLocation().Y > 0.f ? PlateN : PlateS) = *It;
		}
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
		auto WaitGateOpen = [this, Gate](const TCHAR* Why)
		{
			WaitUntil(Why, [Gate]() { return Gate->GetOpenAlpha() > 0.95f; }, 15.f);
		};

		// Take 1 (crate trick): push the prop crate onto the north mark, stand on the south mark.
		MoveTo(200, 180, 20.f);
		MoveTo(200, 545, 40.f, 10.f);
		WaitUntil(TEXT("prop crate presses the north plate"), [PlateN]() { return PlateN->IsActive(); }, 5.f);
		Check(TEXT("one plate is not enough: gate still shut"), [Gate]() { return Gate->GetOpenAlpha() < 0.05f; });
		MoveTo(200, -700, 40.f);
		WaitGateOpen(TEXT("both plates -> gate open"));
		Wait(0.5f);
		Cut();
		// Take 2: ghost 1 repeats the crate trick; lure the guard away.
		NextTake(2);
		MoveTo(700, 0);
		WaitGateOpen(TEXT("ghost 1 holds both marks -> gate open"));
		Check(TEXT("ghost pushed the prop crate onto the plate again"), [PlateN, PropCrate]() { return PlateN->IsActive() && PropCrate != nullptr; });
		MoveTo(1900, -110);
		MoveTo(2700, -600);
		MoveTo(3500, 250);
		WaitUntil(TEXT("guard spots the player"), [Guard]() { return Guard->GetGuardState() == EStuntGuardState::Chase; }, 8.f);
		MoveTo(3800, -1400, 80.f);
		MoveTo(2150, -1400, 80.f); // keep running west, away from the laser door (P4)
		WaitUntil(TEXT("guard catches up (or 8 s)"), [this]() { return Director->GetState() != ERetakeSceneState::Rolling; }, 8.f);
		CutIfRolling();
		// Take 3: lever, lasers, crate bridge, rooftop.
		NextTake(3);
		Check(TEXT("two ghosts spawned"), [this]() { return Director->GetGhosts().Num() == 2; });
		MoveTo(700, 0);
		WaitGateOpen(TEXT("gate open again"));
		MoveTo(1900, -110);
		WaitUntil(TEXT("ghost 2 lures the guard"), [Guard]() { return Guard->GetGuardState() != EStuntGuardState::Patrol; }, 15.f);
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
	}	else if (Scene == TEXT("Scene05_TheGap"))
	{
		// P11: two takes drop into the pit and stay standing; the third hops across their heads.
		auto GhostX = [this](int32 Index) { return Director->GetGhosts().IsValidIndex(Index) ? Director->GetGhosts()[Index]->GetActorLocation().X : 0.f; };
		auto RunJumpOnto = [this](const FString& Name, TFunction<float()> TargetX)
		{
			TSharedPtr<int32> Phase = MakeShared<int32>(0);
			Steps.Add({ Name, 8.f, [this, TargetX, Phase](float)
			{
				if (Director->GetState() == ERetakeSceneState::Wrap) return EStepResult::Done;
				if (Director->GetState() != ERetakeSceneState::Rolling) return EStepResult::Failed;
				UCharacterMovementComponent* Move = Player->GetCharacterMovement();
				Player->AddMovementInput(FVector(1.f, 0.f, 0.f), 0.5f); // half speed (250 cm/s) -> ~306 cm jumps
				const float JumpAt = TargetX() - 306.f;
				if (*Phase == 0 && Move->IsMovingOnGround() && Player->GetActorLocation().X >= JumpAt && Move->Velocity.Size2D() > 220.f)
				{
					Player->StartJump();
					*Phase = 1;
				}
				else if (*Phase == 1 && Move->IsFalling())
				{
					*Phase = 2;
				}
				else if (*Phase == 2 && Move->IsMovingOnGround())
				{
					return EStepResult::Done;
				}
				return EStepResult::Running;
			} });
		};
		// Take 1: drop in, stand at the far end of the pit.
		MoveTo(1550, 0, 30.f);
		Wait(0.5f);
		Cut();
		// Take 2: drop in, stand at the near end.
		NextTake(2);
		MoveTo(1250, 0, 30.f);
		Wait(0.5f);
		Cut();
		// Take 3: wait until both ghosts are frozen, then hop across their heads.
		NextTake(3);
		WaitUntil(TEXT("both ghosts frozen (heads are platforms now)"), [this]()
		{
			for (const ATakeGhost* G : Director->GetGhosts()) { if (!G->HasFinished()) return false; }
			return Director->GetGhosts().Num() == 2;
		}, 10.f);
		MoveTo(600, 0);
		RunJumpOnto(TEXT("RunJump onto ghost 2's head"), [GhostX]() { return GhostX(1); });
		Check(TEXT("standing on a ghost's head"), [this]() { return Player->GetActorLocation().Z > 50.f; });
		RunJumpOnto(TEXT("RunJump onto ghost 1's head"), [GhostX]() { return GhostX(0); });
		Check(TEXT("standing on the other ghost's head"), [this]() { return Player->GetActorLocation().Z > 50.f; });
		RunJumpOnto(TEXT("RunJump to the far side"), []() { return 1830.f + 30.f; });
		MoveTo(2600, 0);
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
	else if (Scene == TEXT("Scene05_TheGap"))
	{
		MoveTo(2600, 0, 60.f, 40.f); // walks straight into the pit and can't climb out
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
		const int32 Max = Director->MaxTakes;
		for (int32 Take = 2; Take <= Max; ++Take)
		{
			WaitUntil(FString::Printf(TEXT("take %d rolling"), Take), [this, Take]()
			{
				return Director->GetState() == ERetakeSceneState::Rolling && Director->GetCurrentTakeNumber() == Take;
			}, 10.f);
			Wait(0.3f);
			Do(TEXT("Press R (cut)"), [this]() { Player->Cut(); });
		}
		Check(TEXT("out of film: last take scrapped, the others kept"), [this, Max]() { return Director->GetTakes().Num() == Max - 1; });
		WaitUntil(TEXT("last take rolls again"), [this, Max]()
		{
			return Director->GetState() == ERetakeSceneState::Rolling && Director->GetCurrentTakeNumber() == Max;
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
		Player->OnActionPerformed.AddUniqueDynamic(this, &ARetakeTestDriver::HandlePlayerAction);
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
	// D1 idle metric (only while a take rolls; restarts per take).
	if (Director->GetState() == ERetakeSceneState::Rolling)
	{
		const int32 Take = Director->GetCurrentTakeNumber();
		if (Take != IdleTake)
		{
			IdleTake = Take;
			IdleNow = 0.f;
			MaxIdlePerTake.Add(Take, 0.f); // a re-rolled take starts over
		}
		const bool bIdle = Player->GetVelocity().Size2D() < 20.f && !bActedThisTick;
		IdleNow = bIdle ? IdleNow + DeltaSeconds : 0.f;
		float& Max = MaxIdlePerTake.FindOrAdd(Take);
		Max = FMath::Max(Max, IdleNow);
	}
	else
	{
		IdleTake = 0;
	}
	bActedThisTick = false;
	if (Director->GetState() == ERetakeSceneState::DirectorsCut)
	{
		if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			bSawCineCamera |= Cast<ACineCameraActor>(PC->GetViewTarget()) != nullptr;
		}
		DirectorsCutRealSeconds += FApp::GetDeltaTime(); // undilated
		// Optional: screenshots of the reel for visual checks (-RetakeReelShots, needs a renderer).
		static float NextShot = 0.f;
		if (FParse::Param(FCommandLine::Get(), TEXT("RetakeReelShots")) && DirectorsCutRealSeconds >= NextShot)
		{
			NextShot = DirectorsCutRealSeconds + 1.f;
			if (APlayerController* ShotPC = GetWorld()->GetFirstPlayerController())
			{
				ShotPC->ConsoleCommand(TEXT("HighResShot 1"));
			}
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

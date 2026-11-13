#include "Retake/RetakeTestDriver.h"

#include "CineCameraActor.h"
#include "Camera/CameraActor.h"
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
#include "Retake/CameraMark.h"
#include "Retake/DirectorsCutComponent.h"
#include "Retake/ExplosiveProp.h"
#include "Retake/RetakeResultsWidget.h"
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
#include "Retake/TimedSwitch.h"

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
	// F1 helper: dump every kept take's path once a second (used to place camera marks).
	Do(TEXT("log take paths"), [this]()
	{
		for (const FTakeData& T : Director->GetTakes())
		{
			FString Line;
			float Next = 0.f;
			for (const FTakeFrame& F : T.Frames)
			{
				if (F.Time >= Next)
				{
					Line += FString::Printf(TEXT(" %.0f:(%.0f,%.0f,%.0f)"), F.Time, F.Location.X, F.Location.Y, F.Location.Z);
					Next += 1.f;
				}
			}
			UE_LOG(LogRetakeTest, Display, TEXT("  path take %d:%s"), T.TakeNumber, *Line);
		}
	});
	// F1: the intended solution hits at least one camera mark (if the scene has marks).
	Check(TEXT("solution hits at least one camera mark"), [this]()
	{
		const FDirectorRating R = Director->GetRating();
		UE_LOG(LogRetakeTest, Display, TEXT("  rating %d (wrap %d, par %d, note %d, marks %d = %d of %d hit)"), R.Total(), R.Wrap, R.Par, R.Note, R.Marks, R.MarksHit, Director->GetMarks().Num());
		return Director->GetMarks().Num() == 0 || R.MarksHit >= 1;
	});
	// G4: some scenes play the Director's Cut by themselves (scene 1 first time, reel ends, finale), the rest go
	// straight to the results.
	TSharedPtr<bool> ExpectAutoCut = MakeShared<bool>(false);
	TSharedPtr<bool> SawAutoCut = MakeShared<bool>(false);
	Do(TEXT("remember whether this wrap plays the cut by itself"), [this, ExpectAutoCut]() { *ExpectAutoCut = Director->ShouldAutoPlayDirectorsCut(); });
	WaitUntil(TEXT("results (through the automatic Director's Cut where there is one)"), [this, SawAutoCut]()
	{
		*SawAutoCut |= Director->GetState() == ERetakeSceneState::DirectorsCut;
		return Director->GetState() == ERetakeSceneState::Results;
	}, 60.f);
	Check(TEXT("automatic Director's Cut played exactly where it should"), [ExpectAutoCut, SawAutoCut]()
	{
		UE_LOG(LogRetakeTest, Display, TEXT("  auto cut expected %d, played %d"), *ExpectAutoCut ? 1 : 0, *SawAutoCut ? 1 : 0);
		return *ExpectAutoCut == *SawAutoCut;
	});
	if (RetakeScenes::IsFinale(Scene))
	{
		Check(TEXT("finale: the memorial card comes up before the results"), [this]()
		{
			const APlayerController* PC = GetWorld()->GetFirstPlayerController();
			const ARetakeHUD* Hud = PC ? PC->GetHUD<ARetakeHUD>() : nullptr;
			return Hud && Hud->GetResultsWidget() && Hud->GetResultsWidget()->IsMemorialShowing();
		});
	}
	WaitUntil(TEXT("rating meter heads for the rating and ends on PRINT IT!"), [this]()
	{
		const APlayerController* PC = GetWorld()->GetFirstPlayerController();
		const ARetakeHUD* Hud = PC ? PC->GetHUD<ARetakeHUD>() : nullptr;
		const URetakeResultsWidget* W = Hud ? Hud->GetResultsWidget() : nullptr;
		return W && W->GetTargetRating() == Director->GetRating().Total() && W->GetFinalPopup() == TEXT("PRINT IT!");
	}, 6.f);	Check(TEXT("stars: 1 wrap, 2 par, 3 par + director's note"), [this, Scene]()
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
	if (RetakeScenes::IsFinale(Scene) && FParse::Param(FCommandLine::Get(), TEXT("RetakeUiShots")))
	{
		Wait(7.5f); // let the memorial card type out for the screenshots
	}
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
	if (Scene == TEXT("Scene04_Finale"))
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
		const int32 MarkShots = Director->DirectorsCut ? Director->DirectorsCut->GetMarkShotsUsed() : 0;
		UE_LOG(LogRetakeTest, Display, TEXT("  camera-mark shots in the reel: %d"), MarkShots);
		return DirectorsCutRealSeconds > 2.f && DirectorsCutRealSeconds < 20.f && (Director->GetMarks().Num() == 0 || MarkShots >= 1);
	});
	Check(TEXT("back on the results, save written once"), [this, Scene]()
	{
		const URetakeSaveGame* Save = URetakeSaveGame::LoadOrCreate();
		return Director->GetState() == ERetakeSceneState::Results && Save && Save->IsWrapped(Scene);
	});
	if (FParse::Param(FCommandLine::Get(), TEXT("RetakeUiShots")))
	{
		Wait(3.f); // let the rating meter play out for the screenshots
	}
	Do(TEXT("Press P (pause)"), [this]() { Player->TogglePause(); });
	Check(TEXT("game paused"), [this]() { return UGameplayStatics::IsGamePaused(this); });
	Do(TEXT("Press P (resume)"), [this]() { Player->TogglePause(); });
	Check(TEXT("game resumed"), [this]() { return !UGameplayStatics::IsGamePaused(this); });
	if (Scene == TEXT("Scene01_Bridge"))
	{
		// K1: RESTART SCENE in the middle of the Director's Cut hands back camera, time and input.
		Do(TEXT("Press Space (watch the cut again)"), [this]() { Player->StartJump(); });
		WaitState((uint8)ERetakeSceneState::DirectorsCut, 3.f);
		Wait(1.f);
		Do(TEXT("Pause -> RESTART SCENE during the cut"), [this]() { Director->RestartScene(); });
		Check(TEXT("restart: cut stopped, normal time, camera on the stunt double, take 1 rolling"), [this]()
		{
			const APlayerController* PC = GetWorld()->GetFirstPlayerController();
			return !Director->DirectorsCut->IsRunning() && FMath::IsNearlyEqual(UGameplayStatics::GetGlobalTimeDilation(this), 1.f)
				&& PC && PC->GetViewTarget() == Player && Director->GetState() == ERetakeSceneState::Rolling
				&& Director->GetCurrentTakeNumber() == 1 && Director->GetTakes().Num() == 0 && !Player->IsHidden();
		});
	}
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
	if (Director->bLocationScout)
	{
		// F7: walk the set during the briefing; nothing may be recorded, and rolling puts us back on the start mark.
		Steps.Add({ TEXT("scout: walk the set during the briefing"), 8.f, [this](float)
		{
			if (Director->GetState() != ERetakeSceneState::Briefing) return EStepResult::Failed;
			const FVector2D Delta = FVector2D(400.f, -300.f) - FVector2D(Player->GetActorLocation());
			if (Delta.Size() < 60.f) return EStepResult::Done;
			Player->AddMovementInput(FVector(Delta.GetSafeNormal(), 0.f), 1.f);
			return EStepResult::Running;
		} });
		Check(TEXT("scouting: still briefing, no take recorded"), [this]()
		{
			return Director->GetState() == ERetakeSceneState::Briefing && Director->GetTakes().Num() == 0 && Director->GetTotalTakesShot() == 0;
		});
	}
	Roll();
	NextTake(1);
	if (Director->bLocationScout)
	{
		Check(TEXT("take 1 starts from the start mark"), [this]() { return FVector::Dist2D(Player->GetActorLocation(), FVector::ZeroVector) < 50.f; });
	}

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
		// D4: PYRO / GRIP / HERO.
		AExplosiveProp* Bomb1 = nullptr;
		AExplosiveProp* Bomb2 = nullptr;
		for (TActorIterator<AExplosiveProp> It(GetWorld()); It; ++It) { (It->GetActorLocation().X < 2200.f ? Bomb1 : Bomb2) = *It; }
		AGate* Exit = FindFirst<AGate>();
		AActor* Wall2 = nullptr;
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (It->ActorHasTag(TEXT("BreakableWall")) && It->GetActorLocation().X > 2500.f) { Wall2 = *It; }
		}
		// Take 1: PYRO.
		MoveTo(1480, 40, 40.f);
		Interact();
		Check(TEXT("charge 1 armed"), [Bomb1]() { return Bomb1->IsArmed(); });
		MoveTo(1050, 0, 40.f);
		WaitUntil(TEXT("charge 1 goes off"), [Bomb1]() { return Bomb1->HasExploded(); }, 4.f);
		MoveTo(2780, 40, 40.f);
		Interact();
		WaitUntil(TEXT("pyro taken out by charge 2"), [this]() { return Director->GetState() == ERetakeSceneState::Cut; }, 5.f);
		Check(TEXT("wall 2 broken"), [Wall2]() { return Wall2 && Wall2->IsHidden(); });
		Check(TEXT("take 1 is a stunt death (thrown, Died on last frame)"), [this, Bomb2]()
		{
			const FTakeData& T = Director->GetTakes()[0];
			const float Thrown = FVector::Dist2D(T.Frames.Last().Location, Bomb2->GetActorLocation());
			UE_LOG(LogRetakeTest, Display, TEXT("  thrown to %.0f cm from charge 2"), Thrown);
			return T.EndsInDeath() && T.EndReason == ERetakeCutReason::Killed && Thrown > 250.f;
		});
		// Take 2: GRIP - north corridor to the exit switch.
		NextTake(2);
		Check(TEXT("wall 2 restored for the new take"), [Wall2]() { return Wall2 && !Wall2->IsHidden(); });
		MoveTo(700, 500);
		MoveTo(3830, 500, 40.f);
		Interact();
		WaitUntil(TEXT("exit gate opens"), [Exit]() { return Exit->ShouldBeOpen(); }, 1.f);
		Cut();
		// Take 3: HERO - follow the blasts, make the exit window.
		NextTake(3);
		Check(TEXT("two ghosts spawned"), [this]() { return Director->GetGhosts().Num() == 2; });
		MoveTo(1100, -100);
		WaitUntil(TEXT("pyro ghost's charge 1"), [Bomb1]() { return Bomb1->HasExploded(); }, 6.f);
		MoveTo(2350, -100);
		WaitUntil(TEXT("pyro ghost's charge 2 (ghost flies and stays down)"), [Bomb2]() { return Bomb2->HasExploded(); }, 6.f);
		WaitUntil(TEXT("pyro ghost lies down without collision"), [this]()
		{
			const ATakeGhost* G = Director->GetGhosts().Num() > 0 ? Director->GetGhosts()[0].Get() : nullptr;
			return G && G->HasFinished() && !G->GetActorEnableCollision();
		}, 3.f);
		MoveTo(3900, -50);
		WaitUntil(TEXT("grip ghost's exit window"), [Exit]() { return Exit->GetOpenAlpha() > 0.95f; }, 3.f);
		MoveTo(4400, 0);
	}	else if (Scene == TEXT("Scene14_ChaseFinal"))
	{
		// Reel 4 remix of scene 3, tighter: GRIP presses the 4 s exit a little later.
		AExplosiveProp* Bomb1 = nullptr;
		AExplosiveProp* Bomb2 = nullptr;
		for (TActorIterator<AExplosiveProp> It(GetWorld()); It; ++It) { (It->GetActorLocation().X < 2200.f ? Bomb1 : Bomb2) = *It; }
		AGate* Exit = FindFirst<AGate>();
		AActor* Wall2 = nullptr;
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (It->ActorHasTag(TEXT("BreakableWall")) && It->GetActorLocation().X > 2500.f) { Wall2 = *It; }
		}
		// Take 1: PYRO.
		MoveTo(1480, 40, 40.f);
		Interact();
		Check(TEXT("charge 1 armed"), [Bomb1]() { return Bomb1->IsArmed(); });
		MoveTo(1050, 0, 40.f);
		WaitUntil(TEXT("charge 1 goes off"), [Bomb1]() { return Bomb1->HasExploded(); }, 4.f);
		MoveTo(2780, 40, 40.f);
		Interact();
		WaitUntil(TEXT("pyro taken out by charge 2"), [this]() { return Director->GetState() == ERetakeSceneState::Cut; }, 5.f);
		Check(TEXT("wall 2 broken"), [Wall2]() { return Wall2 && Wall2->IsHidden(); });
		Check(TEXT("take 1 is a stunt death (thrown, Died on last frame)"), [this, Bomb2]()
		{
			const FTakeData& T = Director->GetTakes()[0];
			const float Thrown = FVector::Dist2D(T.Frames.Last().Location, Bomb2->GetActorLocation());
			UE_LOG(LogRetakeTest, Display, TEXT("  thrown to %.0f cm from charge 2"), Thrown);
			return T.EndsInDeath() && T.EndReason == ERetakeCutReason::Killed && Thrown > 250.f;
		});
		// Take 2: GRIP - north corridor to the exit switch.
		NextTake(2);
		Check(TEXT("wall 2 restored for the new take"), [Wall2]() { return Wall2 && !Wall2->IsHidden(); });
		MoveTo(700, 500);
		MoveTo(3830, 500, 40.f);
		Steps.Add({ TEXT("pace by the switch until 9.6 s"), 10.f, [this](float)
		{
			if (Director->GetState() != ERetakeSceneState::Rolling) return EStepResult::Failed;
			if (Director->TakeDurationSeconds - Director->GetTakeTimeRemaining() >= 9.6f) return EStepResult::Done;
			const float Phase = FMath::Fmod(Director->TakeDurationSeconds - Director->GetTakeTimeRemaining(), 1.2f);
			Player->AddMovementInput(FVector(Phase < 0.6f ? -1.f : 1.f, 0.f, 0.f), 0.5f);
			return EStepResult::Running;
		} });
		MoveTo(3830, 500, 40.f);
		Interact();
		WaitUntil(TEXT("exit gate opens"), [Exit]() { return Exit->ShouldBeOpen(); }, 1.f);
		Cut();
		// Take 3: HERO - follow the blasts, make the exit window.
		NextTake(3);
		Check(TEXT("two ghosts spawned"), [this]() { return Director->GetGhosts().Num() == 2; });
		MoveTo(1100, -100);
		WaitUntil(TEXT("pyro ghost's charge 1"), [Bomb1]() { return Bomb1->HasExploded(); }, 6.f);
		MoveTo(2350, -100);
		WaitUntil(TEXT("pyro ghost's charge 2 (ghost flies and stays down)"), [Bomb2]() { return Bomb2->HasExploded(); }, 6.f);
		WaitUntil(TEXT("pyro ghost lies down without collision"), [this]()
		{
			const ATakeGhost* G = Director->GetGhosts().Num() > 0 ? Director->GetGhosts()[0].Get() : nullptr;
			return G && G->HasFinished() && !G->GetActorEnableCollision();
		}, 3.f);
		MoveTo(3900, -50);
		WaitUntil(TEXT("grip ghost's exit window"), [Exit]() { return Exit->GetOpenAlpha() > 0.95f; }, 3.f);
		MoveTo(4400, 0);
	}	else if (Scene == TEXT("Scene04_Finale"))
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

		// D5: the cast in a working ORDER - GRIP (gate), DECOY (guard), INSIDE MAN (lever), HERO.
		// Take 1: GRIP - prop crate onto the north mark, stand on the south mark.
		MoveTo(200, 180, 20.f);
		MoveTo(200, 390, 40.f, 10.f);
		WaitUntil(TEXT("prop crate presses the north plate"), [PlateN]() { return PlateN->IsActive(); }, 5.f);
		Check(TEXT("one plate is not enough: gate still shut"), [Gate]() { return Gate->GetOpenAlpha() < 0.05f; });
		MoveTo(200, -350, 40.f);
		WaitGateOpen(TEXT("both plates -> gate open"));
		Cut();
		// Take 2: DECOY.
		NextTake(2);
		MoveTo(700, 0);
		WaitGateOpen(TEXT("grip ghost holds both marks -> gate open"));
		MoveTo(1900, -110);
		MoveTo(2500, 300);
		WaitUntil(TEXT("guard spots the decoy"), [Guard]() { return Guard->GetGuardState() == EStuntGuardState::Chase; }, 3.f);
		// Lead it in a circle (a slower pursuer never closes on a circle) until the take runs out.
		Steps.Add({ TEXT("decoy circles the south courtyard until the take ends"), 40.f, [this](float)
		{
			if (Director->GetState() != ERetakeSceneState::Rolling) return EStepResult::Done;
			const FVector2D Center(3000.f, -1000.f);
			const FVector2D P = FVector2D(Player->GetActorLocation()) - Center;
			const float Ang = FMath::Atan2(P.Y, P.X) + FMath::DegreesToRadians(40.f);
			const FVector2D Aim = Center + FVector2D(FMath::Cos(Ang), FMath::Sin(Ang)) * 450.f;
			Player->AddMovementInput(FVector((Aim - FVector2D(Player->GetActorLocation())).GetSafeNormal(), 0.f), 1.f);
			return EStepResult::Running;
		} });
		CutIfRolling();
		// Take 3: INSIDE MAN.
		NextTake(3);
		MoveTo(700, 0);
		WaitGateOpen(TEXT("gate open again"));
		MoveTo(1900, -110);
		MoveTo(2100, 1050, 60.f); // hug the west wall while the decoy pulls the guard south
		WaitUntil(TEXT("decoy ghost has led the guard south"), [Guard]() { return Guard->GetGuardState() != EStuntGuardState::Patrol && Guard->GetActorLocation().Y < -400.f; }, 8.f);
		MoveTo(2900, 960, 40.f);
		Check(TEXT("margin: guard still busy and far from the lever"), [Guard, Lever]()
		{
			return Guard->GetGuardState() != EStuntGuardState::Patrol && FVector::Dist2D(Guard->GetActorLocation(), Lever->GetActorLocation()) > 1200.f;
		});
		Interact();
		Check(TEXT("lever on"), [Lever]() { return Lever->IsActive(); });
		Check(TEXT("lasers off"), LasersOff);
		Cut();
		// Take 4: HERO.
		NextTake(4);
		Check(TEXT("three ghosts spawned"), [this]() { return Director->GetGhosts().Num() == 3; });
		MoveTo(700, 0);
		WaitGateOpen(TEXT("gate open for the hero"));
		MoveTo(1900, -110);
		MoveTo(2080, 800); // hug the west wall while the decoy pulls the guard south
		WaitUntil(TEXT("decoy ghost has led the guard south"), [Guard]() { return Guard->GetGuardState() != EStuntGuardState::Patrol && Guard->GetActorLocation().Y < -400.f; }, 8.f);
		MoveTo(2700, 350); // keep clear of the decoy's circle in the south
		MoveTo(3900, 350);
		WaitUntil(TEXT("inside man ghost kills the lasers"), LasersOff, 6.f);
		MoveTo(4200, 0);
		MoveTo(4560, 0, 30.f);
		MoveTo(5500, 0, 40.f, 15.f);
		WaitUntil(TEXT("crate dropped into the pit"), [Crate]() { return Crate->GetActorLocation().Z < -60.f; }, 8.f);
		MoveTo(6250, 0);
		MoveTo(7150, 0);
		MoveTo(7600, 0);
	}	else if (Scene == TEXT("Scene08_BridgeReshoot"))
	{
		// ST2: one gate needs switch A AND switch B (4 s each) - two grips time their presses, the hero crosses.
		AGate* Gate = FindFirst<AGate>();
		ATimedSwitch* SwitchA = nullptr;
		ATimedSwitch* SwitchB = nullptr;
		for (TActorIterator<ATimedSwitch> It(GetWorld()); It; ++It)
		{
			(It->GetActorLocation().X < 1000.f ? SwitchA : SwitchB) = *It;
		}
		auto TakeTime = [this]() { return Director->TakeDurationSeconds - Director->GetTakeTimeRemaining(); };
		// Walk small loops around (X, Y) until the take clock reaches T (busy, never idle).
		auto MillUntil = [this, TakeTime](float X, float Y, float T)
		{
			TSharedPtr<int32> Leg = MakeShared<int32>(0);
			Steps.Add({ FString::Printf(TEXT("mill around (%.0f, %.0f) until %.1f s"), X, Y, T), 20.f, [this, X, Y, T, Leg, TakeTime](float)
			{
				if (Director->GetState() != ERetakeSceneState::Rolling) return EStepResult::Failed;
				if (TakeTime() >= T) return EStepResult::Done;
				const FVector2D Target(X + (*Leg % 2 == 0 ? 130.f : -130.f), Y);
				const FVector2D Delta = Target - FVector2D(Player->GetActorLocation());
				if (Delta.Size() < 40.f) { ++*Leg; }
				Player->AddMovementInput(FVector(Delta.GetSafeNormal(), 0.f), 0.6f);
				return EStepResult::Running;
			} });
		};
		// Take 1: GRIP - switch A, pressed late so its window overlaps switch B's.
		MoveTo(300, -380, 40.f);
		MillUntil(300, -330, 5.0f);
		MoveTo(300, -440, 30.f);
		Interact();
		Check(TEXT("switch A on, but one switch is not enough"), [SwitchA, Gate]() { return SwitchA->IsActive() && Gate->GetOpenAlpha() < 0.05f; });
		Cut();
		// Take 2: BEST BOY - under the bridge to switch B.
		NextTake(2);
		MoveTo(900, 700);
		MoveTo(2600, 640, 40.f);
		Interact();
		WaitUntil(TEXT("A (ghost) + B -> gate opens"), [Gate]() { return Gate->GetOpenAlpha() > 0.95f; }, 3.f);
		Cut();
		// Take 3: HERO - time the run up the ramp, cross in the overlap.
		NextTake(3);
		MillUntil(-100, 0, 2.2f);
		MoveTo(950, 0);
		MoveTo(1650, 0);
		WaitUntil(TEXT("both switches on -> gate open for the hero"), [Gate]() { return Gate->GetOpenAlpha() > 0.95f; }, 4.f);
		MoveTo(2650, 0);
	}	else if (Scene == TEXT("Scene10_EmptyStage"))
	{
		// ST2: two stand-ins hold the tape marks (plates), the stunt walks through the door.
		AGate* Door = FindFirst<AGate>();
		MoveTo(600, -650, 30.f);
		Check(TEXT("one mark is not enough"), [Door]() { return Door->GetOpenAlpha() < 0.05f; });
		Cut();
		NextTake(2);
		MoveTo(600, 650, 30.f);
		WaitUntil(TEXT("both marks taken -> door opens"), [Door]() { return Door->GetOpenAlpha() > 0.95f; }, 3.f);
		Cut();
		NextTake(3);
		MoveTo(900, 0);
		WaitUntil(TEXT("door open for the stunt"), [Door]() { return Door->GetOpenAlpha() > 0.95f; }, 4.f);
		MoveTo(1550, 0);
	}	else if (Scene == TEXT("Scene12_TheRig"))
	{
		// ST2: RIGGER bridges the pit with the crate, GRIP hits the far-away exit switch late, HERO crosses.
		AGate* Gate = FindFirst<AGate>();
		APushCrate* Crate = FindCrate(true);
		auto TakeTime = [this]() { return Director->TakeDurationSeconds - Director->GetTakeTimeRemaining(); };
		TSharedPtr<float> DropTime = MakeShared<float>(5.f);
		auto MillUntil = [this, TakeTime](float X, float Y, TFunction<float()> UntilT)
		{
			TSharedPtr<int32> Leg = MakeShared<int32>(0);
			Steps.Add({ FString::Printf(TEXT("mill around (%.0f, %.0f)"), X, Y), 20.f, [this, X, Y, UntilT, Leg, TakeTime](float)
			{
				if (Director->GetState() != ERetakeSceneState::Rolling) return EStepResult::Failed;
				if (TakeTime() >= UntilT()) return EStepResult::Done;
				const FVector2D Target(X + (*Leg % 2 == 0 ? 130.f : -130.f), Y);
				const FVector2D Delta = Target - FVector2D(Player->GetActorLocation());
				if (Delta.Size() < 40.f) { ++*Leg; }
				Player->AddMovementInput(FVector(Delta.GetSafeNormal(), 0.f), 0.6f);
				return EStepResult::Running;
			} });
		};
		// Take 1: RIGGER.
		MoveTo(560, 0, 30.f);
		MoveTo(2100, 0, 40.f, 15.f);
		WaitUntil(TEXT("crate drops into the pit"), [Crate]() { return Crate->GetActorLocation().Z < -60.f; }, 6.f);
		Do(TEXT("remember when the bridge was in"), [this, TakeTime, DropTime]() { *DropTime = TakeTime(); UE_LOG(LogRetakeTest, Display, TEXT("  bridge in at %.1f s"), *DropTime); });
		Cut();
		// Take 2: GRIP - exit switch, a few seconds after the bridge is in (the hero still has to cross).
		NextTake(2);
		MoveTo(300, -520, 40.f);
		MillUntil(300, -500, [DropTime]() { return *DropTime + 2.5f; });
		MoveTo(300, -560, 30.f);
		Interact();
		Check(TEXT("exit switch on"), [Gate]() { return Gate->ShouldBeOpen(); });
		Cut();
		// Take 3: HERO.
		NextTake(3);
		MillUntil(700, 500, [DropTime]() { return *DropTime + 0.2f; });
		WaitUntil(TEXT("rigger ghost has bridged the pit"), [Crate]() { return Crate->GetActorLocation().Z < -60.f; }, 3.f);
		MoveTo(1300, 0);
		MoveTo(2400, 0);
		WaitUntil(TEXT("grip ghost holds the exit open"), [Gate]() { return Gate->GetOpenAlpha() > 0.95f; }, 4.f);
		MoveTo(3450, 0);
	}	else if (Scene == TEXT("Scene15_Rehearsal"))
	{
		// ST2: one stand-in becomes the net (frozen in the pit), the stunt hops across its head.
		auto RunJumpTo = [this](const FString& Name, TFunction<float()> TargetX)
		{
			TSharedPtr<int32> Phase = MakeShared<int32>(0);
			Steps.Add({ Name, 8.f, [this, TargetX, Phase](float)
			{
				if (Director->GetState() == ERetakeSceneState::Wrap) return EStepResult::Done;
				if (Director->GetState() != ERetakeSceneState::Rolling) return EStepResult::Failed;
				UCharacterMovementComponent* Move = Player->GetCharacterMovement();
				Player->AddMovementInput(FVector(1.f, 0.f, 0.f), 0.5f); // 250 cm/s -> ~306 cm jumps
				if (*Phase == 0 && Move->IsMovingOnGround() && Player->GetActorLocation().X >= TargetX() - 306.f && Move->Velocity.Size2D() > 220.f)
				{
					Player->StartJump();
					*Phase = 1;
				}
				else if (*Phase == 1 && Move->IsFalling()) { *Phase = 2; }
				else if (*Phase == 2 && Move->IsMovingOnGround()) { return EStepResult::Done; }
				return EStepResult::Running;
			} });
		};
		// Take 1: NET - drop in, stand in the middle.
		MoveTo(1280, 0, 30.f);
		Wait(0.5f);
		Cut();
		// Take 2: STUNT.
		NextTake(2);
		MoveTo(450, 0);
		WaitUntil(TEXT("the net has frozen in place"), [this]() { return Director->GetGhosts().Num() == 1 && Director->GetGhosts()[0]->HasFinished(); }, 6.f);
		MoveTo(700, 0);
		RunJumpTo(TEXT("RunJump onto the net's head"), [this]() { return Director->GetGhosts()[0]->GetActorLocation().X; });
		Check(TEXT("standing on the stand-in's head"), [this]() { return Player->GetActorLocation().Z > 50.f; });
		RunJumpTo(TEXT("RunJump to the far side"), []() { return 1560.f + 30.f; });
		MoveTo(2300, 0);
	}	else if (Scene == TEXT("Scene05_Rooftop"))
	{
		// Reel 2 extra: TECH kills the lasers from the far corner, HERO runs the door.
		auto BeamsOff = [this]()
		{
			for (TActorIterator<ALaserBeam> It(GetWorld()); It; ++It) { if (It->IsBeamOn()) return false; }
			return true;
		};
		MoveTo(-470, -570, 30.f);
		Interact();
		Check(TEXT("lasers off"), BeamsOff);
		Cut();
		NextTake(2);
		MoveTo(1250, 0);
		WaitUntil(TEXT("tech ghost has the lasers off"), BeamsOff, 3.f);
		MoveTo(2400, 0);
	}	else if (Scene == TEXT("Scene07_Vault"))
	{
		// Reel 2 extra: lobby lever + plate 1, plate 2, vault lever + walk in (order matters).
		AGate* AnteGate = nullptr;
		AGate* Vault = nullptr;
		for (TActorIterator<AGate> It(GetWorld()); It; ++It)
		{
			(It->GetActorLocation().X < 1400.f ? AnteGate : Vault) = *It;
		}
		MoveTo(300, 640, 30.f);
		Interact();
		WaitUntil(TEXT("lobby lever opens the antechamber"), [AnteGate]() { return AnteGate->GetOpenAlpha() > 0.95f; }, 3.f);
		MoveTo(700, 0);
		MoveTo(1200, 600, 30.f);
		Cut();
		NextTake(2);
		MoveTo(600, 0);
		WaitUntil(TEXT("inside man ghost has the gate open"), [AnteGate]() { return AnteGate->GetOpenAlpha() > 0.95f; }, 4.f);
		MoveTo(1000, 0);
		MoveTo(1200, -600, 30.f);
		Check(TEXT("both plates held, but the vault still wants its lever"), [Vault]() { return Vault->GetOpenAlpha() < 0.05f; });
		Cut();
		NextTake(3);
		MoveTo(600, 0);
		WaitUntil(TEXT("gate open again"), [AnteGate]() { return AnteGate->GetOpenAlpha() > 0.95f; }, 4.f);
		MoveTo(1500, 300);
		MoveTo(1880, 640, 30.f);
		WaitUntil(TEXT("both plate ghosts in place"), [this]() { return Director->GetGhosts().Num() == 2 && Director->GetGhosts()[1]->HasFinished(); }, 6.f);
		Interact();
		WaitUntil(TEXT("vault door opens"), [Vault]() { return Vault->GetOpenAlpha() > 0.95f; }, 3.f);
		MoveTo(1900, 0);
		MoveTo(2450, 0);
	}	else if (Scene == TEXT("Scene13_ProjectionRoom"))
	{
		// Reel 3 closer: the tight route - A + C in take 1, B + exit in take 2 (par 2).
		AGate* Exit = FindFirst<AGate>();
		MoveTo(300, 840, 30.f);
		Interact();
		MoveTo(1000, 520, 30.f);
		Interact();
		Check(TEXT("two levers are not enough"), [Exit]() { return Exit->GetOpenAlpha() < 0.05f; });
		Cut();
		NextTake(2);
		MoveTo(300, -840, 30.f);
		Interact();
		MoveTo(950, -100);
		WaitUntil(TEXT("all three levers -> exit open"), [Exit]() { return Exit->GetOpenAlpha() > 0.95f; }, 3.f);
		MoveTo(1500, 0);
	}	else if (Scene == TEXT("Scene11_HeistNight"))
	{
		// Reel 3 remix: two decoys circle their guards, inside man, hero.
		AStuntGuard* Guard1 = nullptr;
		AStuntGuard* Guard2 = nullptr;
		for (TActorIterator<AStuntGuard> It(GetWorld()); It; ++It) { (It->GetActorLocation().Y > -500.f ? Guard1 : Guard2) = *It; }
		AGate* Vault = FindFirst<AGate>();
		auto CircleUntilEnd = [this](FVector2D Center, float Radius)
		{
			Steps.Add({ TEXT("circle until the take ends"), 20.f, [this, Center, Radius](float)
			{
				if (Director->GetState() != ERetakeSceneState::Rolling) return EStepResult::Done;
				const FVector2D P = FVector2D(Player->GetActorLocation()) - Center;
				const float Ang = FMath::Atan2(P.Y, P.X) + FMath::DegreesToRadians(40.f);
				const FVector2D Aim = Center + FVector2D(FMath::Cos(Ang), FMath::Sin(Ang)) * Radius;
				Player->AddMovementInput(FVector((Aim - FVector2D(Player->GetActorLocation())).GetSafeNormal(), 0.f), 1.f);
				return EStepResult::Running;
			} });
		};
		// Take 1: DECOY - guard 1, circle in the south-west.
		MoveTo(500, 350);
		WaitUntil(TEXT("guard 1 spots the decoy"), [Guard1]() { return Guard1->GetGuardState() == EStuntGuardState::Chase; }, 3.f);
		CircleUntilEnd(FVector2D(150.f, -900.f), 650.f); // margin R*(1 - 380/500) = 156 > catch distance 110
		// Take 2: SECOND DECOY - guard 2, circle in the south-east.
		NextTake(2);
		MoveTo(1600, -500);
		WaitUntil(TEXT("guard 2 spots the second decoy"), [Guard2]() { return Guard2->GetGuardState() == EStuntGuardState::Chase; }, 3.f);
		CircleUntilEnd(FVector2D(2850.f, -1060.f), 530.f); // margin 127; stays clear of the vault's south wall
		// Take 3: INSIDE MAN.
		NextTake(3);
		MoveTo(-300, 900);
		MoveTo(1000, 1180, 40.f);
		Interact();
		MoveTo(1300, 1180, 40.f);
		Interact();
		MoveTo(1600, 1180, 40.f);
		Interact();
		WaitUntil(TEXT("all three locks on -> vault open"), [Vault]() { return Vault->ShouldBeOpen(); }, 1.f);
		Cut();
		// Take 4: HERO.
		NextTake(4);
		Check(TEXT("three ghosts spawned"), [this]() { return Director->GetGhosts().Num() == 3; });
		// North route (behind guard 1's back), then wait in the north-east until the vault guard takes the bait.
		MoveTo(-200, 700);
		MoveTo(600, 1000);
		MoveTo(2300, 800);
		WaitUntil(TEXT("both guards busy with decoys"), [Guard1, Guard2]()
		{
			return Guard1->GetGuardState() != EStuntGuardState::Patrol && Guard2->GetGuardState() != EStuntGuardState::Patrol;
		}, 4.f);
		MoveTo(2300, -100);
		WaitUntil(TEXT("ghost locks open the vault"), [Vault]() { return Vault->GetOpenAlpha() > 0.95f; }, 3.f);
		MoveTo(3000, 0);
	}	else if (Scene == TEXT("Scene06_Saloon"))
	{
		// Reel 2 extra: one decoy circles BOTH guards; the stunt pushes the crate to the balcony and climbs.
		APushCrate* Crate = FindCrate(true);
		auto BothChasing = [this]()
		{
			int32 N = 0;
			for (TActorIterator<AStuntGuard> It(GetWorld()); It; ++It) { N += It->GetGuardState() != EStuntGuardState::Patrol ? 1 : 0; }
			return N == 2;
		};
		// Take 1: DECOY.
		MoveTo(300, 0);
		WaitUntil(TEXT("both guards spot the decoy"), BothChasing, 3.f);
		Steps.Add({ TEXT("circle until the take ends"), 20.f, [this](float)
		{
			if (Director->GetState() != ERetakeSceneState::Rolling) return EStepResult::Done;
			const FVector2D Center(-350.f, -300.f);
			const FVector2D P = FVector2D(Player->GetActorLocation()) - Center;
			const float Ang = FMath::Atan2(P.Y, P.X) + FMath::DegreesToRadians(40.f);
			const FVector2D Aim = Center + FVector2D(FMath::Cos(Ang), FMath::Sin(Ang)) * 600.f;
			Player->AddMovementInput(FVector((Aim - FVector2D(Player->GetActorLocation())).GetSafeNormal(), 0.f), 1.f);
			return EStepResult::Running;
		} });
		// Take 2: STUNT.
		NextTake(2);
		MoveTo(-200, 950);
		WaitUntil(TEXT("decoy ghost has both guards"), BothChasing, 4.f);
		MoveTo(700, 950);
		MoveTo(1350, 600, 30.f);
		MoveTo(2050, 600, 70.f, 12.f); // pushing the crate east until it hits the balcony (we stop ~117 cm short of its centre)
		Check(TEXT("crate against the balcony"), [Crate]() { return Crate->GetActorLocation().X > 2080.f; });
		// Climb: walk into it and jump whenever something blocks us (floor -> crate -> balcony).
		TSharedPtr<float> Blocked = MakeShared<float>(0.f);
		Steps.Add({ TEXT("climb crate -> balcony"), 8.f, [this, Blocked](float Dt)
		{
			if (Director->GetState() == ERetakeSceneState::Wrap) return EStepResult::Done;
			if (Director->GetState() != ERetakeSceneState::Rolling) return EStepResult::Failed;
			if (Player->GetActorLocation().Z > 330.f) return EStepResult::Done;
			UCharacterMovementComponent* Move = Player->GetCharacterMovement();
			Player->AddMovementInput(FVector(1.f, 0.f, 0.f), 1.f);
			*Blocked = (Move->IsMovingOnGround() && Move->Velocity.Size2D() < 120.f) ? *Blocked + Dt : 0.f;
			if (*Blocked > 0.1f) { Player->StartJump(); *Blocked = -0.4f; }
			return EStepResult::Running;
		} });
		MoveTo(2400, 600);
	}	else if (Scene == TEXT("Scene09_Backlot"))
	{
		// Reel 3 opener (location scout): DECOY circles the guard, PYRO blows the boarded door, HERO goes through.
		AStuntGuard* Guard = FindFirst<AStuntGuard>();
		AExplosiveProp* Charge = FindFirst<AExplosiveProp>();
		auto TakeTime = [this]() { return Director->TakeDurationSeconds - Director->GetTakeTimeRemaining(); };
		// Take 1: DECOY.
		MoveTo(300, 500);
		WaitUntil(TEXT("guard spots the decoy"), [Guard]() { return Guard->GetGuardState() == EStuntGuardState::Chase; }, 3.f);
		Steps.Add({ TEXT("circle until the take ends"), 20.f, [this](float)
		{
			if (Director->GetState() != ERetakeSceneState::Rolling) return EStepResult::Done;
			const FVector2D Center(-250.f, 400.f);
			const FVector2D P = FVector2D(Player->GetActorLocation()) - Center;
			const float Ang = FMath::Atan2(P.Y, P.X) + FMath::DegreesToRadians(40.f);
			const FVector2D Aim = Center + FVector2D(FMath::Cos(Ang), FMath::Sin(Ang)) * 560.f;
			Player->AddMovementInput(FVector((Aim - FVector2D(Player->GetActorLocation())).GetSafeNormal(), 0.f), 1.f);
			return EStepResult::Running;
		} });
		// Take 2: PYRO.
		NextTake(2);
		MoveTo(500, -500);
		WaitUntil(TEXT("decoy ghost has the guard"), [Guard]() { return Guard->GetGuardState() != EStuntGuardState::Patrol; }, 3.f);
		MoveTo(1620, -300, 30.f);
		Interact();
		Check(TEXT("charge armed"), [Charge]() { return Charge->IsArmed(); });
		MoveTo(1100, -750);
		WaitUntil(TEXT("the boarded door blows"), [Charge]() { return Charge->HasExploded(); }, 4.f);
		Cut();
		// Take 3: HERO.
		NextTake(3);
		MoveTo(500, -500);
		MoveTo(1150, -700);
		TSharedPtr<int32> Leg = MakeShared<int32>(0);
		Steps.Add({ TEXT("pace clear of the blast until it goes off"), 12.f, [this, Charge, Leg](float)
		{
			if (Director->GetState() != ERetakeSceneState::Rolling) return EStepResult::Failed;
			if (Charge->HasExploded()) return EStepResult::Done;
			const FVector2D Target((*Leg % 2 == 0) ? 1000.f : 1250.f, -750.f);
			const FVector2D Delta = Target - FVector2D(Player->GetActorLocation());
			if (Delta.Size() < 40.f) { ++*Leg; }
			Player->AddMovementInput(FVector(Delta.GetSafeNormal(), 0.f), 0.6f);
			return EStepResult::Running;
		} });
		MoveTo(1650, -250);
		MoveTo(2800, -250);
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
	else if (Scene == TEXT("Scene09_Backlot"))
	{
		MoveTo(2800, -250, 60.f, 40.f); // the facade is shut and the guard is watching
	}
	else if (Scene == TEXT("Scene06_Saloon"))
	{
		MoveTo(2400, 600, 60.f, 40.f); // busted, or stuck below the balcony
	}
	else if (Scene == TEXT("Scene14_ChaseFinal"))
	{
		MoveTo(200, -700, 40.f);
		MoveTo(4350, 0, 60.f, 40.f); // walls and a shut gate
	}
	else if (Scene == TEXT("Scene11_HeistNight"))
	{
		MoveTo(1900, -250);
		MoveTo(3000, 0, 60.f, 40.f); // guards + a locked vault
	}
	else if (Scene == TEXT("Scene13_ProjectionRoom"))
	{
		MoveTo(300, 840, 30.f);
		Do(TEXT("Press E (lever A)"), [this]() { Player->Interact(); });
		MoveTo(1500, 0, 60.f, 40.f); // one lever: exit stays shut, the take runs out
	}
	else if (Scene == TEXT("Scene07_Vault"))
	{
		MoveTo(300, 640, 30.f);
		Do(TEXT("Press E (lobby lever)"), [this]() { Player->Interact(); });
		MoveTo(700, 0);
		MoveTo(2450, 0, 60.f, 40.f); // the vault stays shut
	}
	else if (Scene == TEXT("Scene05_Rooftop"))
	{
		MoveTo(-470, -570, 30.f);
		Do(TEXT("Press E (lasers off)"), [this]() { Player->Interact(); });
		MoveTo(2400, 0, 60.f, 40.f); // the lasers come back before the door
	}
	else if (Scene == TEXT("Scene15_Rehearsal"))
	{
		MoveTo(2300, 0, 60.f, 40.f); // straight into the pit, no way out
	}
	else if (Scene == TEXT("Scene12_TheRig"))
	{
		MoveTo(300, -560, 30.f);
		Do(TEXT("Press E (exit switch)"), [this]() { Player->Interact(); });
		MoveTo(1300, 0);
		MoveTo(3450, 0, 60.f, 40.f); // no bridge: into the pit
	}
	else if (Scene == TEXT("Scene10_EmptyStage"))
	{
		MoveTo(600, -650, 30.f);
		MoveTo(1550, 0, 60.f, 40.f); // one mark held only while standing on it
	}
	else if (Scene == TEXT("Scene08_BridgeReshoot"))
	{
		MoveTo(300, -440, 30.f);
		Do(TEXT("Press E (switch A alone)"), [this]() { Player->Interact(); });
		MoveTo(100, 0);
		MoveTo(1050, 0);
		MoveTo(2650, 0, 60.f, 40.f); // one switch never opens the gate
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
	// -RetakeLookAt=x,y,z,tx,ty,tz: look at set dressing from a fixed camera, take one shot, then carry on.
	FString LookAt;
	if (LookAtStage < 3 && FParse::Value(FCommandLine::Get(), TEXT("RetakeLookAt="), LookAt, false))
	{
		TArray<FString> N;
		LookAt.ParseIntoArray(N, TEXT(","));
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		if (N.Num() == 6 && PC)
		{
			const FVector From(FCString::Atof(*N[0]), FCString::Atof(*N[1]), FCString::Atof(*N[2]));
			const FVector To(FCString::Atof(*N[3]), FCString::Atof(*N[4]), FCString::Atof(*N[5]));
			if (LookAtStage == 0)
			{
				FActorSpawnParameters P;
				P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				if (ACameraActor* Cam = GetWorld()->SpawnActor<ACameraActor>(From, (To - From).Rotation(), P)) { PC->SetViewTarget(Cam); }
				LookAtClock = FPlatformTime::Seconds();
				LookAtStage = 1;
			}
			else if (LookAtStage == 1 && FPlatformTime::Seconds() - LookAtClock > 2.0)
			{
				PC->ConsoleCommand(TEXT("HighResShot 1920x1080"));
				LookAtStage = 2;
			}
			else if (LookAtStage == 2 && FPlatformTime::Seconds() - LookAtClock > 3.0)
			{
				if (PC->GetPawn()) { PC->SetViewTarget(PC->GetPawn()); }
				LookAtStage = 3;
			}
			if (LookAtStage < 3) { return; } // hold the test until the shot is taken
		}
	}	// F4: -RetakeUiShots: frames with the HUD in them (popping stamp on take 1, rating popups on the results).
	if (UiShots < 3 && FParse::Param(FCommandLine::Get(), TEXT("RetakeUiShots")))
	{
		const double Now = FPlatformTime::Seconds();
		const ERetakeSceneState S = Director->GetState();
		// Results shots after the Director's Cut (the test watches it first), so the meter has time to fill.
		const bool bArm = (UiShots == 0 && S == ERetakeSceneState::Rolling) || (UiShots >= 1 && S == ERetakeSceneState::Results && (DirectorsCutRealSeconds > 1.f || RetakeScenes::IsFinale(Director->GetSceneId())));
		if (bArm && UiShotClock < 0.0)
		{
			UiShotClock = Now;
		}
		const double Delay = UiShots == 0 ? 1.7 : UiShots == 1 ? 3.0 : 6.5; // 1.7 s: the director's story line is up
		if (bArm && Now - UiShotClock >= Delay)
		{
			++UiShots;
			if (UiShots != 2) { UiShotClock = -1.0; }
			if (APlayerController* ShotPC = GetWorld()->GetFirstPlayerController())
			{
				ShotPC->ConsoleCommand(TEXT("Shot showui"));
			}
		}
	}	// F3: -RetakeBlastShot takes 1080p screenshots just after the first explosion of a take (needs a renderer).
	if (BlastShots < 2 && Director->GetState() == ERetakeSceneState::Rolling && FParse::Param(FCommandLine::Get(), TEXT("RetakeBlastShot")))
	{
		const double Now = FPlatformTime::Seconds();
		if (BlastSeenAt < 0.0)
		{
			for (TActorIterator<AExplosiveProp> It(GetWorld()); It; ++It)
			{
				if (It->HasExploded())
				{
					BlastSeenAt = Now;
					// Frame the blast from a low angle a few metres away so the shot shows the effect, not the player's back.
					const FVector C = It->GetActorLocation();
					const FVector CamLoc = C + FVector(-520.f, -60.f, 160.f);
					FActorSpawnParameters P;
					P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					if (ACameraActor* Cam = GetWorld()->SpawnActor<ACameraActor>(CamLoc, (C + FVector(0.f, 0.f, 120.f) - CamLoc).Rotation(), P))
					{
						if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) { PC->SetViewTarget(Cam); }
					}
					break;
				}
			}
		}
		if (BlastSeenAt >= 0.0 && Now - BlastSeenAt >= (BlastShots == 0 ? 0.15 : 0.6))
		{
			++BlastShots;
			if (APlayerController* ShotPC = GetWorld()->GetFirstPlayerController())
			{
				ShotPC->ConsoleCommand(TEXT("HighResShot 1920x1080"));
				if (BlastShots == 2 && ShotPC->GetPawn()) { ShotPC->SetViewTarget(ShotPC->GetPawn()); }
			}
		}
	}	if (Director->GetState() == ERetakeSceneState::DirectorsCut)
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
				ShotPC->ConsoleCommand(TEXT("Shot showui")); // with the overlay: letterbox, scratches
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

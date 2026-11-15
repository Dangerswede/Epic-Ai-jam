#pragma once

#include "CoreMinimal.h"

/**
 * The game's scene list: "The Last Take", 16 scenes in 4 reels (ST1).
 * Story() is the full plan; All() is the playable part of it (rows that have a map), in unlock order.
 * Adding a scene = giving its row a MapName. Map names are short package names under /Game/Retake/Maps.
 * (Godot: an autoload holding an Array of scene Dictionaries.)
 */
namespace RetakeScenes
{
	struct FSceneInfo
	{
		const TCHAR* MapName;         // nullptr = not built yet
		int32 Number;                 // story scene number (1..16)
		int32 Reel;                   // 1..4
		const TCHAR* Title;           // "Scene 1: The Bridge"
		const TCHAR* Logline;         // call sheet menu notes
		const TCHAR* CallSheetHeader; // nullptr = the title in capitals
		const TCHAR* CallSheetNotes;  // one line, <= 60 chars
		const TCHAR* StoryLine;       // director's line after "Quiet on set!" (nullptr = silent)
		bool bEndsReel;               // "END OF REEL N" card after this scene
	};

	inline const TArray<FSceneInfo>& Story()
	{
		static const TArray<FSceneInfo> Scenes = {
			// Reel 1
			{ TEXT("Scene01_Bridge"), 1, 1, TEXT("Scene 1: The Bridge"), TEXT("Two takes. Hold the gate, walk the bridge."), nullptr,
				TEXT("Stunt: J. Ward. Weather: clear. Safety officer: n/a"), TEXT("Alright, people. Same as yesterday. Same as every day."), false },
			{ TEXT("Scene02_Heist"), 2, 1, TEXT("Scene 2: The Heist"), TEXT("Three takes. Distract, disable, grab."), nullptr,
				TEXT("Stunt: J. Ward. Crew call 06:00. Crew present: 0"), TEXT("Has anyone seen Jack? ...Fine. Roll it anyway."), false },
			{ TEXT("Scene03_Chase"), 3, 1, TEXT("Scene 3: The Chase"), TEXT("Blow the wall, bridge the gap."), nullptr,
				TEXT("Stunt: J. Ward. Pyro cleared. Gap rig: NOT INSPECTED"), TEXT("Somebody check that gap rig. Somebody. Anybody?"), false },
			{ TEXT("Scene04_Finale"), 4, 1, TEXT("Scene 4: The Finale"), TEXT("Everything you've learned. Make it a classic."), nullptr,
				TEXT("Stunt: J. Ward. Wrap expected by noon."), TEXT("Take forty-something. I stopped counting. Action."), true },
			// Reel 2
			{ TEXT("Scene05_Rooftop"), 5, 2, TEXT("Scene 5: The Rooftop"), TEXT("Lasers and a timer. The coffee is still hot."), nullptr,
				TEXT("Stunt: J. Ward. Coffee's hot. Nobody's drinking it."), TEXT("Has anyone seen Jack?"), false },
			{ TEXT("Scene06_Saloon"), 6, 2, TEXT("Scene 6: The Saloon"), TEXT("Two guards, a lot of crates."), nullptr,
				TEXT("Stunt: J. Ward. Extras: 0. Wanted posters: 1."), TEXT("Jack loved the saloon gags. Roll it."), false },
			{ TEXT("Scene07_Vault"), 7, 2, TEXT("Scene 7: The Vault"), TEXT("Levers in order. Mind the clock."), nullptr,
				TEXT("Stunt: J. Ward. All clocks stopped at 4:47."), TEXT("What time is it? ...Never mind. Action."), false },
			{ TEXT("Scene08_BridgeReshoot"), 8, 2, TEXT("Scene 8: The Bridge (Reshoot)"), TEXT("Again, at dusk. Three takes this time."), nullptr,
				TEXT("Stunt: J. Ward. Reshoot. Light: fading."), TEXT("Same as yesterday. Same as every day."), true },
			// Reel 3
			{ TEXT("Scene09_Backlot"), 9, 3, TEXT("Scene 9: The Backlot"), TEXT("Night. Nobody is supposed to be here."), nullptr,
				TEXT("Stunt: J. Ward. Location closed. NO SHOOTING TODAY."), TEXT("Nobody's supposed to be out here tonight."), false },
			{ TEXT("Scene10_EmptyStage"), 10, 3, TEXT("Scene 10"), TEXT("Tape marks. One chair."), TEXT("SCENE 10"),
				TEXT("Crew: none. Cast: none. Stunt: J. Ward."), nullptr, false },
			{ TEXT("Scene11_HeistNight"), 11, 3, TEXT("Scene 11: The Heist (Night)"), TEXT("Twice the guards. The lights don't hold."), nullptr,
				TEXT("Stunt: J. Ward. Night shoot. Lights unreliable."), TEXT("Again. In the dark, this time."), false },
			{ TEXT("Scene12_TheRig"), 12, 3, TEXT("Scene 12: The Rig"), TEXT("Crates, a timer, a long drop."), nullptr,
				TEXT("Stunt: J. Ward. Rig: NOT INSPECTED. Sign-off: none."), TEXT("Somebody check that rig. Anybody?"), false },
			{ TEXT("Scene13_ProjectionRoom"), 13, 3, TEXT("Scene 13: The Projection Room"), TEXT("Every take, on loop."), nullptr,
				TEXT("Stunt: J. Ward. Dailies: every take, on loop."), TEXT("I've watched this a thousand times."), true },
			// Reel 4
			{ TEXT("Scene14_ChaseFinal"), 14, 4, TEXT("Scene 14: The Chase (Final)"), TEXT("The last of the pyro. Tighter timing."), nullptr,
				TEXT("Stunt: J. Ward. Pyro: the last of it."), TEXT("One more. Jack always said one more."), false },
			{ TEXT("Scene15_Rehearsal"), 15, 4, TEXT("Scene 15: The Rehearsal"), TEXT("Just a rehearsal. Land on the net."), nullptr,
				TEXT("Safety net: MISSING. Please be careful, Jack."), TEXT("We should have used the net."), false },
			{ TEXT("Scene05_TheGap"), 16, 4, TEXT("Scene 16: The Gap"), TEXT("Stand on your own shoulders."), TEXT("SCENE 99: THE GAP"),
				TEXT("Stunt performer: J. Ward (in memoriam)"), nullptr, false },
		};
		return Scenes;
	}

	/** Playable scenes (built ones) in story order. */
	inline const TArray<FSceneInfo>& All()
	{
		static const TArray<FSceneInfo> Built = []()
		{
			TArray<FSceneInfo> Out;
			for (const FSceneInfo& S : Story())
			{
				if (S.MapName)
				{
					Out.Add(S);
				}
			}
			return Out;
		}();
		return Built;
	}

	inline const TCHAR* MainMenuMap() { return TEXT("MainMenu"); }

	/** Index of a map in All(), or INDEX_NONE. */
	inline int32 IndexOf(const FString& MapName)
	{
		const TArray<FSceneInfo>& Scenes = All();
		for (int32 i = 0; i < Scenes.Num(); ++i)
		{
			if (MapName.Equals(Scenes[i].MapName, ESearchCase::IgnoreCase))
			{
				return i;
			}
		}
		return INDEX_NONE;
	}

	/** Row for a map, or nullptr. */
	inline const FSceneInfo* Find(const FString& MapName)
	{
		const int32 i = IndexOf(MapName);
		return i == INDEX_NONE ? nullptr : &All()[i];
	}

	/** "CALL SHEET - NOV 14, 1977 - SCENE 3: THE CHASE". */
	inline FString CallSheetHeader(const FSceneInfo& S)
	{
		const FString Name = S.CallSheetHeader ? FString(S.CallSheetHeader) : FString(S.Title).ToUpper();
		return FString::Printf(TEXT("CALL SHEET  -  NOV 14, 1977  -  %s"), *Name);
	}

	/** S4 (dropped 15 Nov): displayed take = logic take. Offset read as a bug in playtest, so it is 0. */
	constexpr int32 TakeDisplayOffset = 0;
	inline int32 DisplayTake(int32 TakeNumber) { return TakeNumber + TakeDisplayOffset; }

	/**
	 * ST2 "The Director's Notebook": one dated page per story scene, shown on the results sheet after a wrap.
	 * The dates run from 1977 to 2007: the director has been re-cutting Jack's last day for thirty years.
	 */
	struct FNotebookPage
	{
		const TCHAR* Date;
		const TCHAR* Line;
	};

	/** Notebook page for a story scene number (1..16), or nullptr. */
	inline const FNotebookPage* NotebookPage(int32 SceneNumber)
	{
		static const FNotebookPage Pages[] = {
			{ TEXT("Nov 14, 1977"), TEXT("Jack's first take was perfect. I asked for another one anyway.") },
			{ TEXT("Nov 14, 1977"), TEXT("Jack's girl Annie is on set today. She wants to see her dad fly.") },
			{ TEXT("Nov 14, 1977"), TEXT("Studio cut the safety net from the budget. Jack shrugged: \"Never needed one.\"") },
			{ TEXT("Nov 14, 1977"), TEXT("Behind schedule. Losing the light. Gap jump is next. I signed it off.") },
			{ TEXT("Nov 15, 1977"), TEXT("Nobody came to set today. I came anyway. Made coffee for everyone.") },
			{ TEXT("Dec 2, 1977"),  TEXT("The studio shelved the picture. I kept the reels.") },
			{ TEXT("Nov 14, 1978"), TEXT("One year. Every clock in the house says 4:47. I won't fix them.") },
			{ TEXT("Nov 14, 1979"), TEXT("Recut the bridge scene again. It still doesn't end right.") },
			{ TEXT("Mar 3, 1984"),  TEXT("They're tearing down the backlot. I stood at the fence all night.") },
			{ TEXT("Nov 14, 1991"), TEXT("Annie wrote to me. She's grown now. I haven't answered.") },
			{ TEXT("Nov 14, 1996"), TEXT("I dream about the heist set. The guards walk where Jack walked.") },
			{ TEXT("Nov 14, 2001"), TEXT("Found the rig inspection form in a box. Never signed. Not by anyone. Not by me.") },
			{ TEXT("Nov 14, 2006"), TEXT("I've watched every take a thousand times. In every one, he jumps.") },
			{ TEXT("Nov 13, 2007"), TEXT("Annie called. She asked if she could see the film. Just once.") },
			{ TEXT("Nov 14, 2007"), TEXT("Thirty years. I owe them both an ending. One last cut.") },
			{ TEXT("Nov 14, 2007"), TEXT("Print it. Send it to Annie. Let him rest.") },
		};
		return (SceneNumber >= 1 && SceneNumber <= static_cast<int32>(UE_ARRAY_COUNT(Pages))) ? &Pages[SceneNumber - 1] : nullptr;
	}

	/** "FROM THE DIRECTOR'S NOTEBOOK  -  NOV 14, 1977\n<line>" for a map, or empty. */
	inline FString NotebookText(const FString& MapName)
	{
		const FSceneInfo* S = Find(MapName);
		const FNotebookPage* P = S ? NotebookPage(S->Number) : nullptr;
		return P ? FString::Printf(TEXT("FROM THE DIRECTOR'S NOTEBOOK  -  %s\n%s"), *FString(P->Date).ToUpper(), P->Line) : FString();
	}

	/** The map that ends the story (save flag for the menu's "Production wrapped"). */
	inline const TCHAR* FinalMap() { return TEXT("Scene05_TheGap"); }

	/** S5: the last scene gets the ending (quiet wrap lines, memorial card, FOR JACK cut). */
	inline bool IsFinale(const FString& MapName) { return MapName.Equals(FinalMap(), ESearchCase::IgnoreCase); }
}

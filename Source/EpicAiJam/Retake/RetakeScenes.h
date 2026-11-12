#pragma once

#include "CoreMinimal.h"

/** The game's scene list (order = unlock order). Map names are short package names under /Game/Retake/Maps. */
namespace RetakeScenes
{
	struct FSceneInfo
	{
		const TCHAR* MapName;
		const TCHAR* Title;
		const TCHAR* Logline;
	};

	inline const TArray<FSceneInfo>& All()
	{
		static const TArray<FSceneInfo> Scenes = {
			{ TEXT("Scene01_Bridge"), TEXT("Scene 1: The Bridge"), TEXT("Two takes. Hold the gate, walk the bridge.") },
			{ TEXT("Scene02_Heist"),  TEXT("Scene 2: The Heist"),  TEXT("Three takes. Distract, disable, grab.") },
			{ TEXT("Scene03_Chase"),  TEXT("Scene 3: The Chase"),  TEXT("Big finale energy.") },
		};
		return Scenes;
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
}

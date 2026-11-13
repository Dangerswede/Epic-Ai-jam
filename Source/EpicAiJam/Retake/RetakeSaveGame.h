#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "RetakeSaveGame.generated.h"

/**
 * Scene progress: which scenes are wrapped and the best (fewest) takes per scene.
 * (Godot: a Resource saved with ResourceSaver to user://.)
 */
UCLASS()
class EPICAIJAM_API URetakeSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Map name -> fewest takes used to wrap it. Presence = wrapped. */
	UPROPERTY()
	TMap<FString, int32> BestTakes;

	/** Map name -> most stars earned (0-3). */
	UPROPERTY()
	TMap<FString, int32> BestStars;

	/** Volume settings (0..1), set from the call sheet menu. */
	UPROPERTY() float MusicVolume = 0.8f;
	UPROPERTY() float SfxVolume = 1.f;
	UPROPERTY() float VoiceVolume = 1.f;

	/** Save this object to disk. */
	void SaveNow();

	/** G5: scenes skipped from the pause menu (count as done, 0 stars). */
	UPROPERTY()
	TSet<FString> SkippedScenes;

	bool IsWrapped(const FString& MapName) const { return BestTakes.Contains(MapName) || SkippedScenes.Contains(MapName); }
	bool IsSkippedOnly(const FString& MapName) const { return !BestTakes.Contains(MapName) && SkippedScenes.Contains(MapName); }

	static void RecordSkip(const FString& MapName);

	/** A scene is unlocked if it is the first one or the previous one is wrapped. */
	bool IsUnlocked(int32 SceneIndex) const;

	/** Load (or create) the save. */
	static URetakeSaveGame* LoadOrCreate();

	/** Record a wrap and save to disk. Returns true if it's a new best. */
	static bool RecordWrap(const FString& MapName, int32 TakesUsed, int32 Stars);

	/** Automated playtests (-RetakeTestSave) use their own slot so they never touch real progress. */
	static const TCHAR* SlotName()
	{
		static const bool bTest = FParse::Param(FCommandLine::Get(), TEXT("RetakeTestSave"));
		return bTest ? TEXT("RetakeProgress_Test") : TEXT("RetakeProgress");
	}
};

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

	bool IsWrapped(const FString& MapName) const { return BestTakes.Contains(MapName); }

	/** A scene is unlocked if it is the first one or the previous one is wrapped. */
	bool IsUnlocked(int32 SceneIndex) const;

	/** Load (or create) the save. */
	static URetakeSaveGame* LoadOrCreate();

	/** Record a wrap and save to disk. Returns true if it's a new best. */
	static bool RecordWrap(const FString& MapName, int32 TakesUsed);

	static const TCHAR* SlotName() { return TEXT("RetakeProgress"); }
};

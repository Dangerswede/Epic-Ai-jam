#include "Retake/RetakeSaveGame.h"

#include "Kismet/GameplayStatics.h"
#include "Retake/RetakeScenes.h"

bool URetakeSaveGame::IsUnlocked(int32 SceneIndex) const
{
	if (SceneIndex <= 0)
	{
		return true;
	}
	const TArray<RetakeScenes::FSceneInfo>& Scenes = RetakeScenes::All();
	return Scenes.IsValidIndex(SceneIndex - 1) && IsWrapped(Scenes[SceneIndex - 1].MapName);
}

URetakeSaveGame* URetakeSaveGame::LoadOrCreate()
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName(), 0))
	{
		if (URetakeSaveGame* Save = Cast<URetakeSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName(), 0)))
		{
			return Save;
		}
	}
	return Cast<URetakeSaveGame>(UGameplayStatics::CreateSaveGameObject(URetakeSaveGame::StaticClass()));
}

void URetakeSaveGame::SaveNow()
{
	UGameplayStatics::SaveGameToSlot(this, SlotName(), 0);
}

bool URetakeSaveGame::RecordWrap(const FString& MapName, int32 TakesUsed)
{
	URetakeSaveGame* Save = LoadOrCreate();
	if (!Save)
	{
		return false;
	}
	const int32* Existing = Save->BestTakes.Find(MapName);
	const bool bNewBest = !Existing || TakesUsed < *Existing;
	if (bNewBest)
	{
		Save->BestTakes.Add(MapName, TakesUsed);
	}
	UGameplayStatics::SaveGameToSlot(Save, SlotName(), 0);
	return bNewBest;
}

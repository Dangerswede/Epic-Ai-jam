#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RetakeGameMode.generated.h"

/** Default game mode for Retake: spawns ARetakeCharacter as the player pawn. */
UCLASS()
class EPICAIJAM_API ARetakeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARetakeGameMode();
};

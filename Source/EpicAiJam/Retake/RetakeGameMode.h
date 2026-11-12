#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RetakeGameMode.generated.h"

/** Default game mode for Retake scenes: spawns ARetakeCharacter, uses ARetakeHUD. */
UCLASS()
class EPICAIJAM_API ARetakeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARetakeGameMode();
};

/** Game mode for the MainMenu map: no pawn, call sheet menu HUD. */
UCLASS()
class EPICAIJAM_API ARetakeMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARetakeMenuGameMode();
};

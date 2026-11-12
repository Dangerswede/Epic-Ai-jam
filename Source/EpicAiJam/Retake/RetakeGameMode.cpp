#include "Retake/RetakeGameMode.h"

#include "Retake/RetakeCharacter.h"
#include "Retake/RetakeHUD.h"

ARetakeGameMode::ARetakeGameMode()
{
	DefaultPawnClass = ARetakeCharacter::StaticClass();
	HUDClass = ARetakeHUD::StaticClass();
}

ARetakeMenuGameMode::ARetakeMenuGameMode()
{
	DefaultPawnClass = nullptr;
	HUDClass = ARetakeMenuHUD::StaticClass();
}

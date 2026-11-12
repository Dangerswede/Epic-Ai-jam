#include "Retake/RetakeGameMode.h"

#include "Retake/RetakeCharacter.h"

ARetakeGameMode::ARetakeGameMode()
{
	DefaultPawnClass = ARetakeCharacter::StaticClass();
}

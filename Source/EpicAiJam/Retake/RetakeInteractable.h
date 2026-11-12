#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RetakeInteractable.generated.h"

/**
 * Something the player (or a ghost replaying a take) can use with the Interact action.
 * (Godot: like a group "interactable" + a method every member implements.)
 */
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class URetakeInteractable : public UInterface
{
	GENERATED_BODY()
};

class EPICAIJAM_API IRetakeInteractable
{
	GENERATED_BODY()

public:
	/** Called when Interactor (player or ghost) presses Interact within range. */
	virtual void RetakeInteract(AActor* Interactor) = 0;
};

namespace RetakeInteraction
{
	/** Default reach of the Interact action, in cm. */
	constexpr float DefaultRadius = 175.f;

	/**
	 * The one shared Interact code path for the player and ghosts: finds every
	 * IRetakeInteractable actor within Radius of Interactor and calls RetakeInteract on it.
	 * Returns the number of actors that were interacted with.
	 */
	EPICAIJAM_API int32 PerformInteract(AActor* Interactor, float Radius = DefaultRadius);
}

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RetakeResettable.generated.h"

/**
 * Actors that must return to their initial state at the start of every take
 * (plates, gates, levers, crates, guards...).
 * (Godot: like adding a node to a "resettable" group and calling get_tree().call_group("resettable", "reset_for_take").)
 */
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class URetakeResettable : public UInterface
{
	GENERATED_BODY()
};

class EPICAIJAM_API IRetakeResettable
{
	GENERATED_BODY()

public:
	/** Called by ARetakeSceneDirector before every take (and before the Director's Cut). */
	virtual void ResetForTake() = 0;
};

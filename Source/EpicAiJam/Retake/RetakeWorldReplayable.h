#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RetakeWorldReplayable.generated.h"

/**
 * World actors whose motion is simulated live during a take (guard, crate). The director records
 * their transforms at 30 Hz and, in the Director's Cut, plays the winning take's track back instead
 * of re-simulating, so the cinematic shows exactly what happened.
 * (Godot: like recording an AnimationPlayer track from the node's position and playing it back.)
 */
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class URetakeWorldReplayable : public UInterface
{
	GENERATED_BODY()
};

class EPICAIJAM_API IRetakeWorldReplayable
{
	GENERATED_BODY()

public:
	/** In replay mode the actor runs no logic of its own; the director sets its transform. */
	virtual void SetReplayMode(bool bReplay) = 0;

	/** Small extra state recorded per frame (e.g. the guard's state for its sign). */
	virtual uint8 GetReplayExtra() const { return 0; }
	virtual void ApplyReplayExtra(uint8 Extra) {}
};

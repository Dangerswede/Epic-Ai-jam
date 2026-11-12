#pragma once

#include "CoreMinimal.h"
#include "TakeData.generated.h"

/** Discrete actions recorded in a take. Stored as bitflags in FTakeFrame::ActionFlags. */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ERetakeAction : uint8
{
	None     = 0 UMETA(Hidden),
	Jump     = 1 << 0,
	Interact = 1 << 1,
	Push     = 1 << 2,
	Died     = 1 << 3,  // stunt death: set on the last frame of the take
};
ENUM_CLASS_FLAGS(ERetakeAction);

/** Why a take ended. */
UENUM(BlueprintType)
enum class ERetakeCutReason : uint8
{
	PlayerCut,  // player pressed R
	Killed,     // explosion, laser or kill volume
	TimeUp,     // TakeDurationSeconds ran out
	Caught,     // a guard caught the player
	Wrapped,    // reached the goal (the winning take)
};

/** One fixed-rate (30 Hz) sample of an actor. Never raw input or physics state. */
USTRUCT(BlueprintType)
struct EPICAIJAM_API FTakeFrame
{
	GENERATED_BODY()

	/** Seconds since the take started. */
	UPROPERTY(BlueprintReadOnly, Category = "Retake")
	float Time = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Retake")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Retake")
	FRotator Rotation = FRotator::ZeroRotator;

	/** Only used to drive animation on the ghost, never for simulation. */
	UPROPERTY(BlueprintReadOnly, Category = "Retake")
	FVector Velocity = FVector::ZeroVector;

	/** ERetakeAction flags that happened since the previous frame. */
	UPROPERTY(BlueprintReadOnly, Category = "Retake", meta = (Bitmask, BitmaskEnum = "/Script/EpicAiJam.ERetakeAction"))
	uint8 ActionFlags = 0;

	bool HasAction(ERetakeAction Action) const
	{
		return (ActionFlags & static_cast<uint8>(Action)) != 0;
	}
};

/** A full recorded take. */
USTRUCT(BlueprintType)
struct EPICAIJAM_API FTakeData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Retake")
	TArray<FTakeFrame> Frames;

	/** 1-based take number ("Take 1", "Take 2"...). 0 = no take. */
	UPROPERTY(BlueprintReadOnly, Category = "Retake")
	int32 TakeNumber = 0;

	/** Length in seconds (time of the last frame). */
	UPROPERTY(BlueprintReadOnly, Category = "Retake")
	float Duration = 0.f;

	/** How the take ended. */
	UPROPERTY(BlueprintReadOnly, Category = "Retake")
	ERetakeCutReason EndReason = ERetakeCutReason::PlayerCut;

	/** True if the last frame carries the Died action (the ghost ends lying down). */
	bool EndsInDeath() const { return Frames.Num() > 0 && Frames.Last().HasAction(ERetakeAction::Died); }
};

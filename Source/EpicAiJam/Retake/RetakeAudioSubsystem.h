#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Retake/RetakeSceneDirector.h"
#include "RetakeAudioSubsystem.generated.h"

class AExplosiveProp;
class AGate;
class APushCrate;
class ARetakeActivator;
class AStuntGuard;
class UAudioComponent;
class URetakeAudioSet;
class USoundBase;
class USoundClass;

/**
 * Plays all music, director voice lines and SFX by listening to gameplay delegates.
 * Sounds come from URetakeAudioSet (Project Settings → Retake → Audio Set); empty slots are silent.
 * (Godot: an autoload "AudioManager" connected to every gameplay signal.)
 */
UCLASS()
class EPICAIJAM_API URetakeAudioSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** Apply (and optionally save) Music / SFX / Voice volumes (0..1). */
	static void SetVolumes(const UObject* WorldContext, float Music, float Sfx, float Voice, bool bSave);

	/** Play a UI click (used by menus). */
	void PlayUIClick();

	/** Story voice slots by file name (ST1/F5): plays /Game/Retake/Audio/Sounds/<Name> if it was imported from AudioSource/, else stays silent. */
	void PlayNamedVoice(const FString& Name, float Delay = 0.f);

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	enum class ECategory : uint8 { Music, Sfx, Voice };

	void PlaySound(USoundBase* Sound, ECategory Category, float Delay = 0.f);
	void PlayMusic(USoundBase* Music);
	void ApplySavedVolumes();
	USoundClass* ClassFor(ECategory Category) const;

	UFUNCTION() void HandleStateChanged(ERetakeSceneState NewState, ERetakeSceneState OldState);
	UFUNCTION() void HandleTakeStarted(int32 TakeNumber);
	UFUNCTION() void HandleTakeCut(int32 TakeNumber, ERetakeCutReason Reason);
	UFUNCTION() void HandleSceneFailed();
	UFUNCTION() void HandleWrap();
	UFUNCTION() void HandleActivated(ARetakeActivator* Activator);
	UFUNCTION() void HandleDeactivated(ARetakeActivator* Activator);
	UFUNCTION() void HandleGateOpening(AGate* Gate);
	UFUNCTION() void HandleGateClosing(AGate* Gate);
	UFUNCTION() void HandleArmed(AExplosiveProp* Prop);
	UFUNCTION() void HandleExploded(AExplosiveProp* Prop);
	UFUNCTION() void HandleSpotted(AStuntGuard* Guard, AActor* Target);
	UFUNCTION() void HandleCaught(AStuntGuard* Guard, AActor* Target);
	UFUNCTION() void HandleCratePush(APushCrate* Crate);

	UPROPERTY(Transient) TObjectPtr<URetakeAudioSet> Set;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> MusicComponent;
	UPROPERTY(Transient) TObjectPtr<USoundBase> CurrentMusic;

	bool bRestartedFromTop = false;
};

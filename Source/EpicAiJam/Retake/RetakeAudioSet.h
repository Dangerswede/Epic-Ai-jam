#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DeveloperSettings.h"
#include "RetakeAudioSet.generated.h"

class USoundBase;
class USoundClass;
class USoundMix;
class URetakeAudioSet;

/**
 * All sounds of the game in one data asset (DA_RetakeAudio). Every slot is optional: empty = silent.
 * Scripts/import_audio.py fills slots from AudioSource/<SlotName>.wav automatically.
 * (Godot: a Resource with exported AudioStream fields.)
 */
UCLASS(BlueprintType)
class EPICAIJAM_API URetakeAudioSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// --- Mixing -----------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mixing") TObjectPtr<USoundMix> VolumeMix;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mixing") TObjectPtr<USoundClass> MusicClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mixing") TObjectPtr<USoundClass> SFXClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mixing") TObjectPtr<USoundClass> VoiceClass;

	// --- Director voice lines -----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice") TObjectPtr<USoundBase> VoiceAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice") TObjectPtr<USoundBase> VoiceCut;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice") TObjectPtr<USoundBase> VoiceTakeTwo;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice") TObjectPtr<USoundBase> VoiceTakeThree;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice") TObjectPtr<USoundBase> VoiceTakeFour;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice") TObjectPtr<USoundBase> VoiceTakeFive;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice") TObjectPtr<USoundBase> VoiceThatsAWrap;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice") TObjectPtr<USoundBase> VoicePrintIt;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice") TObjectPtr<USoundBase> VoiceFromTheTop;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice") TObjectPtr<USoundBase> VoiceNewStuntDouble;

	// --- Music (looping where it makes sense) -------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music") TObjectPtr<USoundBase> MusicMenu;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music") TObjectPtr<USoundBase> MusicBriefing;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music") TObjectPtr<USoundBase> MusicRollingLoop;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music") TObjectPtr<USoundBase> MusicDirectorsCut;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music") TObjectPtr<USoundBase> MusicResults;

	// --- SFX -----------------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SFX") TObjectPtr<USoundBase> SfxClapperboard;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SFX") TObjectPtr<USoundBase> SfxPlateOn;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SFX") TObjectPtr<USoundBase> SfxPlateOff;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SFX") TObjectPtr<USoundBase> SfxLever;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SFX") TObjectPtr<USoundBase> SfxGateOpen;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SFX") TObjectPtr<USoundBase> SfxGateClose;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SFX") TObjectPtr<USoundBase> SfxExplosiveArm;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SFX") TObjectPtr<USoundBase> SfxExplosion;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SFX") TObjectPtr<USoundBase> SfxGuardAlert;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SFX") TObjectPtr<USoundBase> SfxGuardCatch;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SFX") TObjectPtr<USoundBase> SfxCratePush;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SFX") TObjectPtr<USoundBase> SfxGoal;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SFX") TObjectPtr<USoundBase> SfxUIClick;
};

/** Project settings (Project Settings → Game → Retake): which audio set to use. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Retake"))
class EPICAIJAM_API URetakeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<URetakeAudioSet> AudioSet;

	static URetakeAudioSet* LoadAudioSet();
};

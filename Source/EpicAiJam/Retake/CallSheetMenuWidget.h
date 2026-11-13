#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CallSheetMenuWidget.generated.h"

/** One PLAY button's click target (dynamic delegates need a UFUNCTION per receiver). */
UCLASS()
class URetakeScenePlayProxy : public UObject
{
	GENERATED_BODY()
public:
	int32 SceneIndex = 0;
	TWeakObjectPtr<class UCallSheetMenuWidget> Menu;
	UFUNCTION() void Click();
};

/**
 * Main menu styled as a film call sheet: one row per scene (Play, locked until the
 * previous scene is wrapped, best takes) and Quit. Built in C++.
 * (Godot: a Control scene with a VBoxContainer of rows created in _ready.)
 */
UCLASS()
class EPICAIJAM_API UCallSheetMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildTree();
public:
	void PlayScene(int32 Index);

private:

	UPROPERTY(Transient) TArray<TObjectPtr<URetakeScenePlayProxy>> PlayProxies;
	UFUNCTION() void HandleQuit();
	UFUNCTION() void HandleVolumeChanged(float Value);

	UPROPERTY(Transient) TObjectPtr<class USlider> MusicSlider;
	UPROPERTY(Transient) TObjectPtr<class USlider> SfxSlider;
	UPROPERTY(Transient) TObjectPtr<class USlider> VoiceSlider;
};

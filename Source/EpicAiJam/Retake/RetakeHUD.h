#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Retake/RetakeSceneDirector.h"
#include "RetakeHUD.generated.h"

class URetakeHUDWidget;
class URetakeResultsWidget;
class UCallSheetMenuWidget;

/**
 * Creates the in-game widgets and connects them to the scene director.
 * (Godot: the node that instances the UI scenes and connects signals.)
 */
UCLASS()
class EPICAIJAM_API ARetakeHUD : public AHUD
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<URetakeHUDWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<URetakeResultsWidget> ResultsWidgetClass;

	ARetakeHUD();

	URetakeHUDWidget* GetHUDWidget() const { return HUDWidget; }

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleStateChanged(ERetakeSceneState NewState, ERetakeSceneState OldState);

	UPROPERTY(Transient) TObjectPtr<URetakeHUDWidget> HUDWidget;
	UPROPERTY(Transient) TObjectPtr<URetakeResultsWidget> ResultsWidget;
	TWeakObjectPtr<ARetakeSceneDirector> Director;
};

/** HUD for the MainMenu map: shows the call sheet with a mouse cursor. */
UCLASS()
class EPICAIJAM_API ARetakeMenuHUD : public AHUD
{
	GENERATED_BODY()

public:
	ARetakeMenuHUD();

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UCallSheetMenuWidget> MenuWidgetClass;

protected:
	virtual void BeginPlay() override;
};

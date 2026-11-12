#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CallSheetMenuWidget.generated.h"

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
	void PlayScene(int32 Index);

	UFUNCTION() void HandlePlay0();
	UFUNCTION() void HandlePlay1();
	UFUNCTION() void HandlePlay2();
	UFUNCTION() void HandlePlay3();
	UFUNCTION() void HandleQuit();
};

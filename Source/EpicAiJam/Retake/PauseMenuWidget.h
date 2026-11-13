#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

/**
 * Esc pause menu: Resume / Restart scene / Main menu. Built in C++.
 * (Godot: a Control with process_mode = ALWAYS shown while get_tree().paused = true.)
 */
UCLASS()
class EPICAIJAM_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Toggle pause for the owning player. */
	void Open();
	void Close();
	bool IsOpen() const { return bOpen; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildTree();

	UFUNCTION() void HandleResume();
	UFUNCTION() void HandleRestart();
	UFUNCTION() void HandleMenu();
	UFUNCTION() void HandleSkip();

	UPROPERTY(Transient) TObjectPtr<class UButton> SkipButton;

	bool bOpen = false;
};

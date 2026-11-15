#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Containers/Ticker.h"
#include "RetakeButton.generated.h"

class UWidget;
class UWidgetTree;

/**
 * Menu button with juice: springy grow + tilt on hover, squash on press, colour fade,
 * click/hover sounds and an optional staggered pop-in. Everything is code-driven
 * (no widget animation assets) and runs on real time, so it also works in the paused game.
 * (Godot: a Button with a Tween on mouse_entered / button_down.)
 */
UCLASS()
class EPICAIJAM_API URetakeButton : public UButton
{
	GENERATED_BODY()

public:
	URetakeButton(const FObjectInitializer& ObjectInitializer);

	/** Pop in (scale 0 -> overshoot -> 1) after Delay seconds. */
	void PlayIntro(float Delay);

	FLinearColor IdleColor = FLinearColor(0.15f, 0.15f, 0.15f, 1.f);
	FLinearColor HoverColor = FLinearColor(0.85f, 0.08f, 0.06f, 1.f);
	FLinearColor PressColor = FLinearColor(0.45f, 0.03f, 0.02f, 1.f);

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void BeginDestroy() override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UFUNCTION() void FxHovered();
	UFUNCTION() void FxUnhovered();
	UFUNCTION() void FxPressed();
	UFUNCTION() void FxReleased();
	UFUNCTION() void FxClicked();

	void Wake();
	bool TickFx(float DeltaTime);
	void ApplyVisuals();

	FTSTicker::FDelegateHandle TickHandle;
	bool bHovered = false;
	bool bPressed = false;
	float Scale = 1.f;
	float ScaleVel = 0.f;
	float Angle = 0.f;
	float AngleVel = 0.f;
	float Hover01 = 0.f;
	float Press01 = 0.f;
	/** < 0: waiting to pop in; >= 0: seconds since the pop started; large: done. */
	float IntroTime = 10.f;
};

namespace RetakeUI
{
	/** Staggered pop-in of every URetakeButton in Tree (in tree order). */
	EPICAIJAM_API void PlayButtonIntros(UWidgetTree* Tree, float StartDelay = 0.05f, float Stagger = 0.04f, int32 MaxStaggered = 8);

	/** Gentle pop-in of any widget (panel, sheet): fade + small overshoot. Real time, works while paused. */
	EPICAIJAM_API void PopIn(UWidget* Widget, float Strength = 0.15f, float Delay = 0.f);
}

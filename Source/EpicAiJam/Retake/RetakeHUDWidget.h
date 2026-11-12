#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Retake/RetakeSceneDirector.h"
#include "RetakeHUDWidget.generated.h"

class UTextBlock;
class UWidget;

/**
 * In-game HUD: clapperboard (scene title, TAKE N / MAX), countdown, ACTION!/CUT! stamps,
 * shot list during Briefing. The widget tree is built in C++; a Blueprint child may restyle it.
 * (Godot: a CanvasLayer with Labels updated from the level manager's signals.)
 */
UCLASS()
class EPICAIJAM_API URetakeHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetDirector(ARetakeSceneDirector* InDirector);

	/** Shows a big stamp text that pops in and fades out. */
	UFUNCTION(BlueprintCallable, Category = "Retake")
	void ShowStamp(const FString& Text, FLinearColor Color, float Duration = 1.4f);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildTree();
	void RefreshStatic();

	UFUNCTION()
	void HandleStateChanged(ERetakeSceneState NewState, ERetakeSceneState OldState);

	UFUNCTION()
	void HandleTakeStarted(int32 TakeNumber);

	UFUNCTION()
	void HandleTakeCut(int32 TakeNumber, ERetakeCutReason Reason);

	UFUNCTION()
	void HandleTakeScrapped(int32 TakeNumber);

	UFUNCTION()
	void HandleOutOfFilm();

	TWeakObjectPtr<ARetakeSceneDirector> Director;

	UPROPERTY(Transient) TObjectPtr<UWidget> Clapper;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SceneTitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TakeText;
	UPROPERTY(Transient) TObjectPtr<UWidget> TimerPanel;
	UPROPERTY(Transient) TObjectPtr<UWidget> KeyHints;
	UPROPERTY(Transient) TObjectPtr<UWidget> TimelinePanel;
	UPROPERTY(Transient) TObjectPtr<class UCanvasPanel> TimelineCanvas;
	UPROPERTY(Transient) TObjectPtr<class USizeBox> TimelineSize;
	UPROPERTY(Transient) TObjectPtr<UWidget> Playhead;
	float TimelineWidth = 600.f;
	int32 TimelineRows = 0;

public:
	/** Number of take rows currently shown in the timeline (for tests). */
	int32 GetTimelineRowCount() const { return TimelineRows; }

private:

	void RebuildTimeline();
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TimerText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StampText;
	UPROPERTY(Transient) TObjectPtr<UWidget> BriefingPanel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ShotListText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NoteText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HintText;

	float StampTime = 0.f;
	float StampDuration = 0.f;
};

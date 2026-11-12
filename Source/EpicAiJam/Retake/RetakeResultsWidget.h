#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RetakeResultsWidget.generated.h"

class ARetakeSceneDirector;
class UBorder;
class UButton;
class UTextBlock;

/**
 * Results screen after the Director's Cut: takes used, par, stars (3 if takes <= par),
 * Next scene / Retry / Main menu. Built in C++.
 */
UCLASS()
class EPICAIJAM_API URetakeResultsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowResults(ARetakeSceneDirector* InDirector, bool bNewBest);

	/** F1: rating shown by the meter right now (animates up to the director's rating). */
	int32 GetDisplayedRating() const { return FMath::RoundToInt(DisplayedRating); }

	/** Latest meter popup ("PRINT IT!", "..."). */
	FString GetLastPopup() const { return LastPopup; }

	/** Where the meter is heading and the popup it ends on (checked by the headless playtest). */
	int32 GetTargetRating() const { return FMath::RoundToInt(TargetRating); }
	FString GetFinalPopup() const { return TargetStars >= 3 ? TEXT("PRINT IT!") : TEXT("..."); }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildTree();
	void CloseAndRestoreInput();
	void Popup(const FString& Str, const FLinearColor& Color);
	void SetMeter(float Rating);

	UFUNCTION() void HandleNext();
	UFUNCTION() void HandleRetry();
	UFUNCTION() void HandleMenu();
	UFUNCTION() void HandleWatch();

	TWeakObjectPtr<ARetakeSceneDirector> Director;
	int32 NextSceneIndex = INDEX_NONE;

	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TakesText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BestText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NoteText;
	UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> Stars;
	UPROPERTY(Transient) TObjectPtr<UButton> NextButton;
	UPROPERTY(Transient) TObjectPtr<class USizeBox> MeterFill;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> RatingText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PopupText;

	struct FMeterStep { float UpTo; FString Label; };
	TArray<FMeterStep> MeterSteps;
	int32 NextStep = 0;
	int32 TargetStars = 0;
	float TargetRating = 0.f;
	float DisplayedRating = 0.f;
	float PopupTime = 10.f;
	bool bMeterRunning = false;
	FString LastPopup;
};

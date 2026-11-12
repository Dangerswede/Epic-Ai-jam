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

	/** 3 stars at or under par, 2 at par+1, otherwise 1. */
	static int32 ComputeStars(int32 TakesUsed, int32 Par);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildTree();
	void CloseAndRestoreInput();

	UFUNCTION() void HandleNext();
	UFUNCTION() void HandleRetry();
	UFUNCTION() void HandleMenu();

	TWeakObjectPtr<ARetakeSceneDirector> Director;
	int32 NextSceneIndex = INDEX_NONE;

	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TakesText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BestText;
	UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> Stars;
	UPROPERTY(Transient) TObjectPtr<UButton> NextButton;
};

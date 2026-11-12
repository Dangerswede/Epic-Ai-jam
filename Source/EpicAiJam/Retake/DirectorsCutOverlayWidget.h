#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DirectorsCutOverlayWidget.generated.h"

class UTextBlock;

/** Letterbox bars, a fading title card and a "Space to skip" hint. Built in C++. */
UCLASS()
class EPICAIJAM_API UDirectorsCutOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowTitle(const FString& Title, const FString& Subtitle, float Duration);

	/** Small slate text bottom-left (e.g. "TAKE 2") shown at each cut. */
	void ShowShotLabel(const FString& Label);

	/** Full-screen black frame used while the reel skips dead time. */
	void ShowBlack(bool bBlack);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildTree();

	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SubtitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ShotLabel;
	UPROPERTY(Transient) TObjectPtr<class UBorder> BlackFrame;

	float TitleTime = 0.f;
	float TitleDuration = 0.f;
};

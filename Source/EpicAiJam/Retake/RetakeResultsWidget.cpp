#include "Retake/RetakeResultsWidget.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Retake/RetakeSceneDirector.h"
#include "Retake/RetakeScenes.h"
#include "Retake/RetakeWidgetUtils.h"

int32 URetakeResultsWidget::ComputeStars(int32 TakesUsed, int32 Par)
{
	if (TakesUsed <= Par) return 3;
	if (TakesUsed == Par + 1) return 2;
	return 1;
}

TSharedRef<SWidget> URetakeResultsWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void URetakeResultsWidget::BuildTree()
{
	using namespace RetakeUI;
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Canvas;

	UBorder* Sheet = Panel(WidgetTree, Paper(), FMargin(40.f, 28.f));
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Sheet->SetContent(Box);

	Box->AddChildToVerticalBox(Text(WidgetTree, TEXT("DAILIES REPORT"), 14, Red(), ETextJustify::Center));
	TitleText = Text(WidgetTree, TEXT("Scene"), 34, Ink(), ETextJustify::Center);
	Box->AddChildToVerticalBox(TitleText)->SetPadding(FMargin(0.f, 6.f));

	// Stars as gold/grey diamonds (rotated squares) - no font glyphs needed.
	UHorizontalBox* StarRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	for (int32 i = 0; i < 3; ++i)
	{
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Size->SetWidthOverride(44.f);
		Size->SetHeightOverride(44.f);
		UBorder* Star = Panel(WidgetTree, Dim(), FMargin(0.f));
		Star->SetRenderTransformAngle(45.f);
		Star->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Size->SetContent(Star);
		UHorizontalBoxSlot* S = StarRow->AddChildToHorizontalBox(Size);
		S->SetPadding(FMargin(16.f, 12.f));
		Stars.Add(Star);
	}
	UVerticalBoxSlot* StarSlot = Box->AddChildToVerticalBox(StarRow);
	StarSlot->SetHorizontalAlignment(HAlign_Center);

	TakesText = Text(WidgetTree, TEXT(""), 22, Ink(), ETextJustify::Center, false);
	Box->AddChildToVerticalBox(TakesText)->SetPadding(FMargin(0.f, 6.f));
	BestText = Text(WidgetTree, TEXT(""), 18, Red(), ETextJustify::Center);
	Box->AddChildToVerticalBox(BestText);

	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	NextButton = Button(WidgetTree, TEXT("NEXT SCENE"), 18);
	UButton* Retry = Button(WidgetTree, TEXT("RETRY"), 18);
	UButton* Menu = Button(WidgetTree, TEXT("MAIN MENU"), 18);
	for (UButton* Btn : { NextButton.Get(), Retry, Menu })
	{
		Buttons->AddChildToHorizontalBox(Btn)->SetPadding(FMargin(8.f, 0.f));
	}
	NextButton->OnClicked.AddDynamic(this, &URetakeResultsWidget::HandleNext);
	Retry->OnClicked.AddDynamic(this, &URetakeResultsWidget::HandleRetry);
	Menu->OnClicked.AddDynamic(this, &URetakeResultsWidget::HandleMenu);
	UVerticalBoxSlot* BtnSlot = Box->AddChildToVerticalBox(Buttons);
	BtnSlot->SetHorizontalAlignment(HAlign_Center);
	BtnSlot->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));

	UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Sheet);
	S->SetAnchors(FAnchors(0.5f, 0.5f));
	S->SetAlignment(FVector2D(0.5f, 0.5f));
	S->SetAutoSize(true);
}

void URetakeResultsWidget::ShowResults(ARetakeSceneDirector* InDirector, bool bNewBest)
{
	Director = InDirector;
	if (!InDirector || !TitleText)
	{
		return;
	}
	const int32 Used = InDirector->GetTakes().Num();
	const int32 Par = InDirector->ParTakes;
	const int32 NumStars = ComputeStars(Used, Par);

	TitleText->SetText(InDirector->SceneTitle);
	TakesText->SetText(FText::FromString(FString::Printf(TEXT("Takes in the cut: %d   (par %d)\nFilm shot: %d takes"), Used, Par, InDirector->GetTotalTakesShot())));
	BestText->SetText(FText::FromString(bNewBest ? TEXT("NEW BEST!") : TEXT("")));
	for (int32 i = 0; i < Stars.Num(); ++i)
	{
		Stars[i]->SetBrushColor(i < NumStars ? RetakeUI::Gold() : RetakeUI::Dim());
	}

	const int32 Current = RetakeScenes::IndexOf(InDirector->GetSceneId());
	NextSceneIndex = (Current != INDEX_NONE && RetakeScenes::All().IsValidIndex(Current + 1)) ? Current + 1 : INDEX_NONE;
	NextButton->SetVisibility(NextSceneIndex != INDEX_NONE ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	SetVisibility(ESlateVisibility::Visible);
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(true);
		FInputModeGameAndUI Mode;
		Mode.SetWidgetToFocus(TakeWidget());
		PC->SetInputMode(Mode);
	}
}

void URetakeResultsWidget::CloseAndRestoreInput()
{
	SetVisibility(ESlateVisibility::Collapsed);
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void URetakeResultsWidget::HandleNext()
{
	if (NextSceneIndex == INDEX_NONE)
	{
		return;
	}
	CloseAndRestoreInput();
	UGameplayStatics::OpenLevel(this, FName(RetakeScenes::All()[NextSceneIndex].MapName));
}

void URetakeResultsWidget::HandleRetry()
{
	CloseAndRestoreInput();
	if (ARetakeSceneDirector* D = Director.Get())
	{
		D->RestartScene();
	}
}

void URetakeResultsWidget::HandleMenu()
{
	CloseAndRestoreInput();
	UGameplayStatics::OpenLevel(this, FName(RetakeScenes::MainMenuMap()));
}

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

	// F1: Director's rating meter (fills up and lights the stars at 45 / 65 / 85).
	Box->AddChildToVerticalBox(Text(WidgetTree, TEXT("DIRECTOR'S RATING"), 14, Red(), ETextJustify::Center));
	UHorizontalBox* MeterRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	USizeBox* MeterBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	MeterBox->SetWidthOverride(360.f);
	MeterBox->SetHeightOverride(20.f);
	UBorder* MeterBack = Panel(WidgetTree, FLinearColor(0.15f, 0.13f, 0.11f), FMargin(2.f));
	MeterBox->SetContent(MeterBack);
	UHorizontalBox* FillRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	MeterBack->SetContent(FillRow);
	MeterFill = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	MeterFill->SetWidthOverride(0.f);
	MeterFill->SetContent(Panel(WidgetTree, Gold(), FMargin(0.f)));
	FillRow->AddChildToHorizontalBox(MeterFill);
	MeterRow->AddChildToHorizontalBox(MeterBox)->SetVerticalAlignment(VAlign_Center);
	RatingText = Text(WidgetTree, TEXT("0"), 22, Ink(), ETextJustify::Left, false);
	USizeBox* NumberBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	NumberBox->SetWidthOverride(64.f); // fixed width so the bar doesn't shift when the number grows
	NumberBox->SetContent(RatingText);
	MeterRow->AddChildToHorizontalBox(NumberBox)->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
	UVerticalBoxSlot* MeterSlot = Box->AddChildToVerticalBox(MeterRow);
	MeterSlot->SetHorizontalAlignment(HAlign_Center);
	MeterSlot->SetPadding(FMargin(0.f, 4.f));
	PopupText = PopText(WidgetTree, TEXT(""), 34, Orange());
	PopupText->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	PopupText->SetRenderOpacity(0.f);
	Box->AddChildToVerticalBox(PopupText)->SetPadding(FMargin(0.f, 2.f));

	TakesText = Text(WidgetTree, TEXT(""), 22, Ink(), ETextJustify::Center, false);
	Box->AddChildToVerticalBox(TakesText)->SetPadding(FMargin(0.f, 6.f));
	NoteText = Text(WidgetTree, TEXT(""), 18, Ink(), ETextJustify::Center);
	Box->AddChildToVerticalBox(NoteText)->SetPadding(FMargin(0.f, 4.f));
	BestText = Text(WidgetTree, TEXT(""), 18, Red(), ETextJustify::Center);
	Box->AddChildToVerticalBox(BestText);

	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UButton* Watch = Button(WidgetTree, TEXT("WATCH DIRECTOR'S CUT  [SPACE]"), 18);
	Watch->OnClicked.AddDynamic(this, &URetakeResultsWidget::HandleWatch);
	UVerticalBoxSlot* WatchSlot = Box->AddChildToVerticalBox(Watch);
	WatchSlot->SetHorizontalAlignment(HAlign_Center);
	WatchSlot->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));

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
	const bool bBonus = InDirector->IsBonusMet();
	const FDirectorRating Rating = InDirector->GetRating();
	TargetRating = Rating.Total();
	TargetStars = InDirector->GetStars();
	MeterSteps.Reset();
	float Sum = Rating.Wrap;
	MeterSteps.Add({ Sum, FString::Printf(TEXT("WRAP +%d"), Rating.Wrap) });
	if (Rating.Par > 0) { Sum += Rating.Par; MeterSteps.Add({ Sum, FString::Printf(TEXT("ON PAR +%d"), Rating.Par) }); }
	if (Rating.Note > 0) { Sum += Rating.Note; MeterSteps.Add({ Sum, FString::Printf(TEXT("NOTE +%d"), Rating.Note) }); }
	if (Rating.Marks > 0) { MeterSteps.Add({ TargetRating, FString::Printf(TEXT("ON MARK x%d +%d"), Rating.MarksHit, Rating.Marks) }); }
	NextStep = 0;
	PopupTime = 10.f;
	LastPopup.Reset();
	bMeterRunning = true;
	SetMeter(0.f);
	if (!InDirector->BonusText.IsEmpty())
	{
		NoteText->SetText(FText::FromString(FString::Printf(TEXT("%s DIRECTOR'S NOTE: %s"), bBonus ? TEXT("[X]") : TEXT("[  ]"), *InDirector->BonusText.ToString())));
		NoteText->SetColorAndOpacity(FSlateColor(bBonus ? FLinearColor(0.1f, 0.45f, 0.1f) : RetakeUI::Dim()));
	}
	else
	{
		NoteText->SetText(FText::GetEmpty());
	}

	TitleText->SetText(InDirector->SceneTitle);
	TakesText->SetText(FText::FromString(FString::Printf(TEXT("Takes in the cut: %d   (par %d)\nFilm shot: %d takes"), Used, Par, InDirector->GetTotalTakesShot())));
	BestText->SetText(FText::FromString(bNewBest ? TEXT("NEW BEST!") : TEXT("")));

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

void URetakeResultsWidget::SetMeter(float Rating)
{
	DisplayedRating = Rating;
	if (MeterFill) MeterFill->SetWidthOverride(356.f * FMath::Clamp(Rating / 100.f, 0.f, 1.f));
	if (RatingText) RatingText->SetText(FText::AsNumber(FMath::RoundToInt(Rating)));
	// Stars light as the needle passes their thresholds.
	const int32 Lit = FMath::Min(TargetStars, FDirectorRating::StarsFor(FMath::RoundToInt(Rating), Rating >= FDirectorRating::WrapPoints - 0.5f));
	for (int32 i = 0; i < Stars.Num(); ++i)
	{
		Stars[i]->SetBrushColor(i < Lit ? RetakeUI::Gold() : RetakeUI::Dim());
	}
}

void URetakeResultsWidget::Popup(const FString& Str, const FLinearColor& Color)
{
	LastPopup = Str;
	if (PopupText)
	{
		PopupText->SetText(FText::FromString(Str));
		PopupText->SetColorAndOpacity(FSlateColor(Color));
	}
	PopupTime = 0.f;
}

void URetakeResultsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (bMeterRunning)
	{
		const float Next = FMath::Min(TargetRating, DisplayedRating + 55.f * InDeltaTime);
		SetMeter(Next);
		while (MeterSteps.IsValidIndex(NextStep) && Next >= MeterSteps[NextStep].UpTo - 0.01f)
		{
			Popup(MeterSteps[NextStep].Label, RetakeUI::Orange());
			++NextStep;
		}
		if (Next >= TargetRating - 0.01f && PopupTime > 0.45f)
		{
			bMeterRunning = false;
			Popup(GetFinalPopup(), TargetStars >= 3 ? RetakeUI::Mustard() : RetakeUI::Dim());
		}
	}
	// Pop: scale 0 -> 1.3 -> 1 in 0.25 s with a slight tilt, then hold.
	PopupTime += InDeltaTime;
	if (PopupText)
	{
		PopupText->SetRenderScale(FVector2D(RetakeUI::PopScale(PopupTime)));
		PopupText->SetRenderTransformAngle(RetakeUI::PopTilt(PopupTime, -4.f));
		PopupText->SetRenderOpacity(LastPopup.IsEmpty() ? 0.f : 1.f);
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

void URetakeResultsWidget::HandleWatch()
{
	CloseAndRestoreInput();
	if (ARetakeSceneDirector* D = Director.Get())
	{
		D->WatchDirectorsCut();
	}
}

void URetakeResultsWidget::HandleMenu()
{
	CloseAndRestoreInput();
	UGameplayStatics::OpenLevel(this, FName(RetakeScenes::MainMenuMap()));
}

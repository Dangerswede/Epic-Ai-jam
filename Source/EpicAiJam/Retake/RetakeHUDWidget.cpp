#include "Retake/RetakeHUDWidget.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Retake/RetakeWidgetUtils.h"

TSharedRef<SWidget> URetakeHUDWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void URetakeHUDWidget::BuildTree()
{
	using namespace RetakeUI;
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Canvas;

	// Clapperboard, top-left.
	{
		UBorder* Board = Panel(WidgetTree, Slate(), FMargin(18.f, 10.f));
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Box->AddChildToVerticalBox(Text(WidgetTree, TEXT("RETAKE PRODUCTIONS"), 12, Red()));
		SceneTitleText = Text(WidgetTree, TEXT("Scene"), 20, Chalk());
		Box->AddChildToVerticalBox(SceneTitleText);
		TakeText = Text(WidgetTree, TEXT("TAKE 1 / 5"), 30, Chalk());
		Box->AddChildToVerticalBox(TakeText);
		Board->SetContent(Box);
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Board);
		S->SetAnchors(FAnchors(0.f, 0.f));
		S->SetPosition(FVector2D(32.f, 32.f));
		S->SetAutoSize(true);
		Clapper = Board;
	}

	// Countdown, top-right.
	{
		UBorder* Box = Panel(WidgetTree, Slate(), FMargin(18.f, 10.f));
		TimerText = Text(WidgetTree, TEXT("0:25"), 34, Chalk(), ETextJustify::Center);
		Box->SetContent(TimerText);
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Box);
		S->SetAnchors(FAnchors(1.f, 0.f));
		S->SetAlignment(FVector2D(1.f, 0.f));
		S->SetPosition(FVector2D(-32.f, 32.f));
		S->SetAutoSize(true);
		TimerPanel = Box;
	}

	// Stamp, center.
	{
		StampText = Text(WidgetTree, TEXT(""), 96, Red(), ETextJustify::Center);
		StampText->SetAutoWrapText(false);
		StampText->SetRenderOpacity(0.f);
		StampText->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(StampText);
		S->SetAnchors(FAnchors(0.5f, 0.4f));
		S->SetAlignment(FVector2D(0.5f, 0.5f));
		S->SetAutoSize(true);
	}

	// Briefing / shot list, lower center.
	{
		UBorder* Sheet = Panel(WidgetTree, Paper(), FMargin(28.f, 20.f));
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Box->AddChildToVerticalBox(Text(WidgetTree, TEXT("SHOT LIST"), 14, Red()));
		ShotListText = Text(WidgetTree, TEXT(""), 22, Ink(), ETextJustify::Left, false);
		Box->AddChildToVerticalBox(ShotListText)->SetPadding(FMargin(0.f, 6.f));
		HintText = Text(WidgetTree, TEXT("Press Space to roll"), 18, Ink());
		Box->AddChildToVerticalBox(HintText)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
		Sheet->SetContent(Box);
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Sheet);
		S->SetAnchors(FAnchors(0.5f, 1.f));
		S->SetAlignment(FVector2D(0.5f, 1.f));
		S->SetPosition(FVector2D(0.f, -60.f));
		S->SetSize(FVector2D(720.f, 200.f));
		S->SetAutoSize(true);
		BriefingPanel = Sheet;
	}

	// Key hints, bottom-left, while rolling.
	{
		UBorder* Hints = Panel(WidgetTree, Slate(), FMargin(14.f, 6.f));
		Hints->SetContent(Text(WidgetTree,
			TEXT("WASD Move   SPACE Jump   E Use   R Cut   BACKSPACE Scrap take   ESC Pause"), 14, Chalk(), ETextJustify::Left, false));
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Hints);
		S->SetAnchors(FAnchors(0.f, 1.f));
		S->SetAlignment(FVector2D(0.f, 1.f));
		S->SetPosition(FVector2D(32.f, -24.f));
		S->SetAutoSize(true);
		KeyHints = Hints;
	}
}

void URetakeHUDWidget::SetDirector(ARetakeSceneDirector* InDirector)
{
	if (Director.IsValid())
	{
		Director->OnStateChanged.RemoveDynamic(this, &URetakeHUDWidget::HandleStateChanged);
		Director->OnTakeStarted.RemoveDynamic(this, &URetakeHUDWidget::HandleTakeStarted);
		Director->OnTakeCut.RemoveDynamic(this, &URetakeHUDWidget::HandleTakeCut);
		Director->OnTakeScrapped.RemoveDynamic(this, &URetakeHUDWidget::HandleTakeScrapped);
		Director->OnOutOfFilm.RemoveDynamic(this, &URetakeHUDWidget::HandleOutOfFilm);
	}
	Director = InDirector;
	if (InDirector)
	{
		InDirector->OnStateChanged.AddDynamic(this, &URetakeHUDWidget::HandleStateChanged);
		InDirector->OnTakeStarted.AddDynamic(this, &URetakeHUDWidget::HandleTakeStarted);
		InDirector->OnTakeCut.AddDynamic(this, &URetakeHUDWidget::HandleTakeCut);
		InDirector->OnTakeScrapped.AddDynamic(this, &URetakeHUDWidget::HandleTakeScrapped);
		InDirector->OnOutOfFilm.AddDynamic(this, &URetakeHUDWidget::HandleOutOfFilm);
		HandleStateChanged(InDirector->GetState(), ERetakeSceneState::None);
	}
	RefreshStatic();
}

void URetakeHUDWidget::RefreshStatic()
{
	const ARetakeSceneDirector* D = Director.Get();
	if (!D || !SceneTitleText)
	{
		return;
	}
	SceneTitleText->SetText(D->SceneTitle);
	ShotListText->SetText(D->ShotListText);
	const int32 Take = FMath::Max(1, D->GetCurrentTakeNumber());
	TakeText->SetText(FText::FromString(FString::Printf(TEXT("TAKE %d / %d"), Take, D->MaxTakes)));
	HintText->SetText(FText::FromString(D->bWaitForRollInput ? TEXT("Press Space to roll") : TEXT("Get ready...")));
}

void URetakeHUDWidget::ShowStamp(const FString& Str, FLinearColor Color, float Duration)
{
	if (!StampText)
	{
		return;
	}
	StampText->SetText(FText::FromString(Str));
	StampText->SetColorAndOpacity(FSlateColor(Color));
	StampTime = 0.f;
	StampDuration = Duration;
}

void URetakeHUDWidget::HandleStateChanged(ERetakeSceneState NewState, ERetakeSceneState OldState)
{
	RefreshStatic();
	const bool bBriefing = NewState == ERetakeSceneState::Briefing;
	const bool bHideAll = NewState == ERetakeSceneState::DirectorsCut || NewState == ERetakeSceneState::Results;

	if (BriefingPanel) BriefingPanel->SetVisibility(bBriefing ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (Clapper) Clapper->SetVisibility(bHideAll ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	if (TimerPanel) TimerPanel->SetVisibility(NewState == ERetakeSceneState::Rolling ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (KeyHints) KeyHints->SetVisibility(NewState == ERetakeSceneState::Rolling ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (StampText && bHideAll) StampDuration = 0.f, StampText->SetRenderOpacity(0.f);

	switch (NewState)
	{
	case ERetakeSceneState::SceneFailed:
		ShowStamp(TEXT("SCENE FAILED\nBack to Take 1"), RetakeUI::Red(), 2.8f);
		break;
	case ERetakeSceneState::Wrap:
		ShowStamp(TEXT("THAT'S A WRAP!"), RetakeUI::Gold(), 2.f);
		break;
	default:
		break;
	}
}

void URetakeHUDWidget::HandleTakeStarted(int32 TakeNumber)
{
	RefreshStatic();
	ShowStamp(TEXT("ACTION!"), RetakeUI::Gold(), 1.2f);
}

void URetakeHUDWidget::HandleTakeCut(int32 TakeNumber, ERetakeCutReason Reason)
{
	const TCHAR* Why = TEXT("");
	switch (Reason)
	{
	case ERetakeCutReason::Killed: Why = TEXT("\nOuch."); break;
	case ERetakeCutReason::TimeUp: Why = TEXT("\nToo slow!"); break;
	case ERetakeCutReason::Caught: Why = TEXT("\nBusted."); break;
	default: break;
	}
	ShowStamp(FString::Printf(TEXT("CUT! Take %d%s"), TakeNumber, Why), RetakeUI::Red(), 1.8f);
}

void URetakeHUDWidget::HandleTakeScrapped(int32 TakeNumber)
{
	ShowStamp(FString::Printf(TEXT("SCRAPPED!\nTake %d hits the cutting room floor"), TakeNumber), RetakeUI::Red(), 1.6f);
}

void URetakeHUDWidget::HandleOutOfFilm()
{
	ShowStamp(TEXT("OUT OF FILM!\nLast take scrapped"), RetakeUI::Red(), 1.9f);
}

void URetakeHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (const ARetakeSceneDirector* D = Director.Get())
	{
		if (TimerText && D->GetState() == ERetakeSceneState::Rolling)
		{
			const float Left = D->GetTakeTimeRemaining();
			const int32 Secs = FMath::CeilToInt(Left);
			TimerText->SetText(FText::FromString(FString::Printf(TEXT("%d:%02d"), Secs / 60, Secs % 60)));
			TimerText->SetColorAndOpacity(FSlateColor(Left <= 5.f ? RetakeUI::Red() : RetakeUI::Chalk()));
		}
	}

	if (StampText && StampDuration > 0.f)
	{
		StampTime += InDeltaTime;
		const float T = StampTime / StampDuration;
		// Slam in (scale 2 -> 1 in 0.15 s), hold, fade out over the last 30%.
		const float Scale = FMath::Lerp(2.f, 1.f, FMath::Clamp(StampTime / 0.15f, 0.f, 1.f));
		const float Opacity = T < 0.7f ? 1.f : FMath::Clamp(1.f - (T - 0.7f) / 0.3f, 0.f, 1.f);
		StampText->SetRenderScale(FVector2D(Scale));
		StampText->SetRenderOpacity(Opacity);
		StampText->SetRenderTransformAngle(-6.f);
		if (T >= 1.f)
		{
			StampDuration = 0.f;
			StampText->SetRenderOpacity(0.f);
		}
	}
}

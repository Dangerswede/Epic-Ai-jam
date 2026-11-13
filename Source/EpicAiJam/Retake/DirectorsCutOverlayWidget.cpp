#include "Retake/DirectorsCutOverlayWidget.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Retake/RetakeWidgetUtils.h"

TSharedRef<SWidget> UDirectorsCutOverlayWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UDirectorsCutOverlayWidget::BuildTree()
{
	using namespace RetakeUI;
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Canvas;

	// Black frame behind everything else (shown while skipping between highlights).
	BlackFrame = Panel(WidgetTree, FLinearColor::Black, FMargin(0.f));
	BlackFrame->SetVisibility(ESlateVisibility::Collapsed);
	{
		UCanvasPanelSlot* BS = Canvas->AddChildToCanvas(BlackFrame);
		BS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		BS->SetOffsets(FMargin(0.f));
	}

	// 2.39:1-ish letterbox: 12% bars top and bottom.
	auto Bar = [&](float Top, float Bottom)
	{
		UBorder* B = Panel(WidgetTree, FLinearColor::Black, FMargin(0.f));
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(B);
		S->SetAnchors(FAnchors(0.f, Top, 1.f, Bottom));
		S->SetOffsets(FMargin(0.f));
	};
	Bar(0.f, 0.12f);
	Bar(0.88f, 1.f);

	// F6: replay-only film damage: a flickering dark veil and a few vertical scratches that jump around.
	FlickerVeil = Panel(WidgetTree, FLinearColor::Black, FMargin(0.f));
	FlickerVeil->SetRenderOpacity(0.f);
	FlickerVeil->SetVisibility(ESlateVisibility::HitTestInvisible);
	{
		UCanvasPanelSlot* FS = Canvas->AddChildToCanvas(FlickerVeil);
		FS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		FS->SetOffsets(FMargin(0.f));
	}
	for (int32 i = 0; i < 3; ++i)
	{
		UBorder* Scratch = Panel(WidgetTree, FLinearColor(0.92f, 0.88f, 0.8f, 0.45f), FMargin(0.f));
		Scratch->SetVisibility(ESlateVisibility::Hidden);
		UCanvasPanelSlot* SS2 = Canvas->AddChildToCanvas(Scratch);
		SS2->SetAnchors(FAnchors(0.5f, 0.12f, 0.5f, 0.88f));
		SS2->SetOffsets(FMargin(0.f, 0.f, 2.f, 0.f));
		Scratches.Add(Scratch);
	}

	TitleText = PopText(WidgetTree, TEXT(""), 72, Mustard());
	TitleText->SetRenderOpacity(0.f);
	UCanvasPanelSlot* T = Canvas->AddChildToCanvas(TitleText);
	T->SetAnchors(FAnchors(0.5f, 0.45f));
	T->SetAlignment(FVector2D(0.5f, 0.5f));
	T->SetAutoSize(true);

	SubtitleText = Text(WidgetTree, TEXT(""), 20, Gold(), ETextJustify::Center);
	SubtitleText->SetRenderOpacity(0.f);
	UCanvasPanelSlot* ST = Canvas->AddChildToCanvas(SubtitleText);
	ST->SetAnchors(FAnchors(0.5f, 0.55f));
	ST->SetAlignment(FVector2D(0.5f, 0.5f));
	ST->SetAutoSize(true);

	ShotLabel = Text(WidgetTree, TEXT(""), 16, Chalk(), ETextJustify::Left);
	ShotLabel->SetAutoWrapText(false);
	UCanvasPanelSlot* SL = Canvas->AddChildToCanvas(ShotLabel);
	SL->SetAnchors(FAnchors(0.f, 0.94f));
	SL->SetAlignment(FVector2D(0.f, 0.5f));
	SL->SetPosition(FVector2D(32.f, 0.f));
	SL->SetAutoSize(true);

	UTextBlock* Skip = Text(WidgetTree, TEXT("SPACE: skip"), 14, Dim(), ETextJustify::Right);
	UCanvasPanelSlot* SS = Canvas->AddChildToCanvas(Skip);
	SS->SetAnchors(FAnchors(1.f, 0.94f));
	SS->SetAlignment(FVector2D(1.f, 0.5f));
	SS->SetPosition(FVector2D(-32.f, 0.f));
	SS->SetAutoSize(true);
}

void UDirectorsCutOverlayWidget::ShowTitle(const FString& Title, const FString& Subtitle, float Duration)
{
	if (!TitleText)
	{
		return;
	}
	TitleText->SetText(FText::FromString(Title));
	SubtitleText->SetText(FText::FromString(Subtitle));
	TitleTime = 0.f;
	TitleDuration = Duration;
}

void UDirectorsCutOverlayWidget::ShowShotLabel(const FString& Label)
{
	if (ShotLabel)
	{
		ShotLabel->SetText(FText::FromString(Label));
	}
}

void UDirectorsCutOverlayWidget::ShowBlack(bool bBlack)
{
	if (BlackFrame)
	{
		BlackFrame->SetVisibility(bBlack ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UDirectorsCutOverlayWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	TickFilmDamage(InDeltaTime);
	if (TitleDuration <= 0.f || !TitleText)
	{
		return;
	}
	TitleTime += InDeltaTime;
	// Fade in 0.3 s, hold, fade out 0.4 s.
	const float In = FMath::Clamp(TitleTime / 0.3f, 0.f, 1.f);
	const float Out = FMath::Clamp((TitleDuration - TitleTime) / 0.4f, 0.f, 1.f);
	const float Alpha = FMath::Min(In, Out);
	TitleText->SetRenderOpacity(Alpha);
	TitleText->SetRenderScale(FVector2D(RetakeUI::PopScale(TitleTime)));
	TitleText->SetRenderTransformAngle(RetakeUI::PopTilt(TitleTime, -3.f));
	SubtitleText->SetRenderOpacity(Alpha);
	if (TitleTime >= TitleDuration)
	{
		TitleDuration = 0.f;
		TitleText->SetRenderOpacity(0.f);
		SubtitleText->SetRenderOpacity(0.f);
	}
}

void UDirectorsCutOverlayWidget::TickFilmDamage(float DeltaTime)
{
	DamageClock += DeltaTime;
	if (FlickerVeil)
	{
		FlickerVeil->SetRenderOpacity(FMath::FRandRange(0.f, 0.07f));
	}
	if (DamageClock < 0.09f)
	{
		return;
	}
	DamageClock = 0.f;
	// New frame of "damage": each scratch is shown at a random column with some chance.
	for (UBorder* Scratch : Scratches)
	{
		const bool bShow = FMath::FRand() < 0.35f;
		Scratch->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Scratch->Slot); S && bShow)
		{
			const float X = FMath::FRandRange(0.05f, 0.95f);
			const float Top = FMath::FRandRange(0.12f, 0.5f);
			S->SetAnchors(FAnchors(X, Top, X, FMath::FRandRange(Top + 0.1f, 0.88f)));
			S->SetOffsets(FMargin(0.f, 0.f, FMath::FRandRange(1.f, 3.f), 0.f));
		}
	}
}

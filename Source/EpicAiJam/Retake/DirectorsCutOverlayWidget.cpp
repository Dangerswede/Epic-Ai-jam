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

	TitleText = Text(WidgetTree, TEXT(""), 64, Chalk(), ETextJustify::Center);
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

void UDirectorsCutOverlayWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
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
	SubtitleText->SetRenderOpacity(Alpha);
	if (TitleTime >= TitleDuration)
	{
		TitleDuration = 0.f;
		TitleText->SetRenderOpacity(0.f);
		SubtitleText->SetRenderOpacity(0.f);
	}
}

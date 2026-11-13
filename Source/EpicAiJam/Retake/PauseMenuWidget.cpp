#include "Retake/PauseMenuWidget.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Retake/RetakeSceneDirector.h"
#include "Retake/RetakeScenes.h"
#include "Retake/RetakeWidgetUtils.h"

TSharedRef<SWidget> UPauseMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UPauseMenuWidget::BuildTree()
{
	using namespace RetakeUI;
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Canvas;

	UBorder* Dim = Panel(WidgetTree, FLinearColor(0.f, 0.f, 0.f, 0.6f), FMargin(0.f));
	UCanvasPanelSlot* DS = Canvas->AddChildToCanvas(Dim);
	DS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	DS->SetOffsets(FMargin(0.f));

	UBorder* Sheet = Panel(WidgetTree, Paper(), FMargin(40.f, 28.f));
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Sheet->SetContent(Box);
	Box->AddChildToVerticalBox(Text(WidgetTree, TEXT("HOLD THE ROLL"), 14, Red(), ETextJustify::Center));
	Box->AddChildToVerticalBox(Text(WidgetTree, TEXT("PAUSED"), 44, Ink(), ETextJustify::Center));
	Box->AddChildToVerticalBox(Text(WidgetTree, TEXT("The crew is on a coffee break."), 16, Ink(), ETextJustify::Center, false))
		->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));

	auto AddButton = [&](const TCHAR* Label) -> UButton*
	{
		UButton* B = Button(WidgetTree, Label, 20);
		UVerticalBoxSlot* S = Box->AddChildToVerticalBox(B);
		S->SetPadding(FMargin(0.f, 6.f));
		S->SetHorizontalAlignment(HAlign_Fill);
		return B;
	};
	AddButton(TEXT("RESUME"))->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleResume);
	AddButton(TEXT("RESTART SCENE"))->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleRestart);
	// G5: only shown after running out of film or 3 scrapped takes.
	SkipButton = AddButton(TEXT("SKIP SCENE (0 stars)"));
	SkipButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleSkip);
	AddButton(TEXT("MAIN MENU"))->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleMenu);

	UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Sheet);
	S->SetAnchors(FAnchors(0.5f, 0.5f));
	S->SetAlignment(FVector2D(0.5f, 0.5f));
	S->SetAutoSize(true);
}

void UPauseMenuWidget::Open()
{
	APlayerController* PC = GetOwningPlayer();
	if (bOpen || !PC)
	{
		return;
	}
	bOpen = true;
	if (!IsInViewport())
	{
		AddToViewport(20);
	}
	SetVisibility(ESlateVisibility::Visible);
	if (SkipButton)
	{
		const ARetakeSceneDirector* D = ARetakeSceneDirector::Get(this);
		SkipButton->SetVisibility(D && D->CanSkipScene() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	UGameplayStatics::SetGamePaused(this, true);
	PC->SetShowMouseCursor(true);
	FInputModeGameAndUI Mode;
	Mode.SetWidgetToFocus(TakeWidget());
	Mode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(Mode);
}

void UPauseMenuWidget::Close()
{
	APlayerController* PC = GetOwningPlayer();
	if (!bOpen || !PC)
	{
		return;
	}
	bOpen = false;
	SetVisibility(ESlateVisibility::Collapsed);
	UGameplayStatics::SetGamePaused(this, false);
	// The results screen keeps its own cursor.
	const ARetakeSceneDirector* D = ARetakeSceneDirector::Get(this);
	if (!D || D->GetState() != ERetakeSceneState::Results)
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void UPauseMenuWidget::HandleResume()
{
	Close();
}

void UPauseMenuWidget::HandleRestart()
{
	Close();
	if (ARetakeSceneDirector* D = ARetakeSceneDirector::Get(this))
	{
		D->RestartScene();
	}
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void UPauseMenuWidget::HandleSkip()
{
	Close();
	if (ARetakeSceneDirector* D = ARetakeSceneDirector::Get(this))
	{
		D->SkipScene();
	}
}

void UPauseMenuWidget::HandleMenu()
{
	Close();
	UGameplayStatics::OpenLevel(this, FName(RetakeScenes::MainMenuMap()));
}

#include "Retake/PauseMenuWidget.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Retake/RetakeAudioSubsystem.h"
#include "Retake/RetakeSaveGame.h"
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

	// F7: the sound department, also from inside a scene (same save as the call sheet sliders).
	Box->AddChildToVerticalBox(Text(WidgetTree, TEXT("SOUND DEPARTMENT"), 14, Red()))->SetPadding(FMargin(0.f, 14.f, 0.f, 2.f));
	const URetakeSaveGame* Save = URetakeSaveGame::LoadOrCreate();
	auto AddSlider = [&](const TCHAR* Label, float Value) -> USlider*
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		LabelBox->SetWidthOverride(90.f);
		LabelBox->SetContent(Text(WidgetTree, Label, 15, Ink()));
		Row->AddChildToHorizontalBox(LabelBox)->SetVerticalAlignment(VAlign_Center);
		USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		SB->SetWidthOverride(200.f);
		USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
		Slider->SetValue(Value);
		Slider->SetSliderBarColor(RetakeUI::Dim());
		Slider->SetSliderHandleColor(Red());
		Slider->OnValueChanged.AddDynamic(this, &UPauseMenuWidget::HandleVolumeChanged);
		SB->SetContent(Slider);
		Row->AddChildToHorizontalBox(SB)->SetVerticalAlignment(VAlign_Center);
		Box->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 2.f));
		return Slider;
	};
	MusicSlider = AddSlider(TEXT("MUSIC"), Save ? Save->MusicVolume : 0.8f);
	SfxSlider = AddSlider(TEXT("SFX"), Save ? Save->SfxVolume : 1.f);
	VoiceSlider = AddSlider(TEXT("VOICE"), Save ? Save->VoiceVolume : 1.f);

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

void UPauseMenuWidget::HandleVolumeChanged(float)
{
	URetakeAudioSubsystem::SetVolumes(this, MusicSlider->GetValue(), SfxSlider->GetValue(), VoiceSlider->GetValue(), true);
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

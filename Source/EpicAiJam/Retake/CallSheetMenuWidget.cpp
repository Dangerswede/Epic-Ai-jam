#include "Retake/CallSheetMenuWidget.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/Slider.h"
#include "Retake/RetakeAudioSubsystem.h"
#include "Retake/RetakeSaveGame.h"
#include "Retake/RetakeScenes.h"
#include "Retake/RetakeWidgetUtils.h"

TSharedRef<SWidget> UCallSheetMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UCallSheetMenuWidget::BuildTree()
{
	using namespace RetakeUI;
	const URetakeSaveGame* Save = URetakeSaveGame::LoadOrCreate();

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Canvas;

	// Dark backdrop.
	{
		UBorder* Backdrop = Panel(WidgetTree, FLinearColor(0.03f, 0.03f, 0.035f, 1.f), FMargin(0.f));
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Backdrop);
		S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		S->SetOffsets(FMargin(0.f));
	}

	UBorder* Sheet = Panel(WidgetTree, Paper(), FMargin(48.f, 36.f));
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Sheet->SetContent(Box);

	Box->AddChildToVerticalBox(Text(WidgetTree, TEXT("CALL SHEET"), 16, Red()));
	Box->AddChildToVerticalBox(Text(WidgetTree, TEXT("RETAKE"), 64, Ink()));
	Box->AddChildToVerticalBox(Text(WidgetTree,
		TEXT("Production: Retake  |  Role: Stunt Double (you)  |  Director: very impatient"), 14, Ink(), ETextJustify::Left, false));
	Box->AddChildToVerticalBox(Text(WidgetTree,
		TEXT("Every failed take stays in the shot. Make your past takes work together."), 16, Ink(), ETextJustify::Left, false))
		->SetPadding(FMargin(0.f, 8.f, 0.f, 16.f));

	// Header row.
	auto Row = [this](const FString& A, const FString& B, const FString& C, int32 Size, const FLinearColor& Color)
	{
		UHorizontalBox* H = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		auto Cell = [&](const FString& Str, float Width)
		{
			USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			SB->SetWidthOverride(Width);
			SB->SetContent(RetakeUI::Text(WidgetTree, Str, Size, Color, ETextJustify::Left, Size > 16));
			H->AddChildToHorizontalBox(SB)->SetVerticalAlignment(VAlign_Center);
		};
		Cell(A, 300.f);
		Cell(B, 380.f);
		Cell(C, 260.f);
		return H;
	};
	Box->AddChildToVerticalBox(Row(TEXT("SCENE"), TEXT("NOTES"), TEXT("STATUS"), 14, Red()));

	const TArray<RetakeScenes::FSceneInfo>& Scenes = RetakeScenes::All();
	for (int32 i = 0; i < Scenes.Num() && i < 5; ++i)
	{
		const bool bUnlocked = Save && Save->IsUnlocked(i);
		const int32* Best = Save ? Save->BestTakes.Find(Scenes[i].MapName) : nullptr;
		const int32* Stars = Save ? Save->BestStars.Find(Scenes[i].MapName) : nullptr;
		FString Status = !bUnlocked ? TEXT("LOCKED") : Best
			? FString::Printf(TEXT("WRAPPED %d/3 stars (%d takes)"), Stars ? *Stars : 1, *Best) : TEXT("READY");

		UHorizontalBox* H = Row(Scenes[i].Title, Scenes[i].Logline, Status, 18, bUnlocked ? Ink() : Dim());
		UButton* Play = Button(WidgetTree, bUnlocked ? TEXT("PLAY") : TEXT("---"), 16);
		Play->SetIsEnabled(bUnlocked);
		H->AddChildToHorizontalBox(Play)->SetVerticalAlignment(VAlign_Center);
		// AddDynamic needs a literal function name.
		switch (i)
		{
		case 0: Play->OnClicked.AddDynamic(this, &UCallSheetMenuWidget::HandlePlay0); break;
		case 1: Play->OnClicked.AddDynamic(this, &UCallSheetMenuWidget::HandlePlay1); break;
		case 2: Play->OnClicked.AddDynamic(this, &UCallSheetMenuWidget::HandlePlay2); break;
		case 3: Play->OnClicked.AddDynamic(this, &UCallSheetMenuWidget::HandlePlay3); break;
		default: Play->OnClicked.AddDynamic(this, &UCallSheetMenuWidget::HandlePlay4); break;
		}
		Box->AddChildToVerticalBox(H)->SetPadding(FMargin(0.f, 8.f));
	}

	// Sound department: three volume sliders (saved).
	Box->AddChildToVerticalBox(Text(WidgetTree, TEXT("SOUND DEPARTMENT"), 14, Red()))->SetPadding(FMargin(0.f, 20.f, 0.f, 4.f));
	UHorizontalBox* Sound = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	auto AddSlider = [&](const TCHAR* Label, float Value) -> USlider*
	{
		Sound->AddChildToHorizontalBox(Text(WidgetTree, Label, 16, Ink()))->SetVerticalAlignment(VAlign_Center);
		USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		SB->SetWidthOverride(180.f);
		USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
		Slider->SetValue(Value);
		Slider->SetSliderBarColor(Dim());
		Slider->SetSliderHandleColor(Red());
		SB->SetContent(Slider);
		UHorizontalBoxSlot* HS = Sound->AddChildToHorizontalBox(SB);
		HS->SetVerticalAlignment(VAlign_Center);
		HS->SetPadding(FMargin(8.f, 0.f, 28.f, 0.f));
		return Slider;
	};
	MusicSlider = AddSlider(TEXT("MUSIC"), Save ? Save->MusicVolume : 0.8f);
	SfxSlider = AddSlider(TEXT("SFX"), Save ? Save->SfxVolume : 1.f);
	VoiceSlider = AddSlider(TEXT("VOICE"), Save ? Save->VoiceVolume : 1.f);
	MusicSlider->OnValueChanged.AddDynamic(this, &UCallSheetMenuWidget::HandleVolumeChanged);
	SfxSlider->OnValueChanged.AddDynamic(this, &UCallSheetMenuWidget::HandleVolumeChanged);
	VoiceSlider->OnValueChanged.AddDynamic(this, &UCallSheetMenuWidget::HandleVolumeChanged);
	Box->AddChildToVerticalBox(Sound);

	UButton* Quit = Button(WidgetTree, TEXT("QUIT"), 18);
	Quit->OnClicked.AddDynamic(this, &UCallSheetMenuWidget::HandleQuit);
	UVerticalBoxSlot* QuitSlot = Box->AddChildToVerticalBox(Quit);
	QuitSlot->SetHorizontalAlignment(HAlign_Right);
	QuitSlot->SetPadding(FMargin(0.f, 20.f, 0.f, 0.f));

	Box->AddChildToVerticalBox(Text(WidgetTree, TEXT("All stunts performed by trained professionals. Mostly."), 12, Dim(), ETextJustify::Left, false))
		->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));

	UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Sheet);
	S->SetAnchors(FAnchors(0.5f, 0.5f));
	S->SetAlignment(FVector2D(0.5f, 0.5f));
	S->SetAutoSize(true);
}

void UCallSheetMenuWidget::PlayScene(int32 Index)
{
	const TArray<RetakeScenes::FSceneInfo>& Scenes = RetakeScenes::All();
	if (!Scenes.IsValidIndex(Index))
	{
		return;
	}
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}
	UGameplayStatics::OpenLevel(this, FName(Scenes[Index].MapName));
}

void UCallSheetMenuWidget::HandlePlay0() { PlayScene(0); }
void UCallSheetMenuWidget::HandlePlay1() { PlayScene(1); }
void UCallSheetMenuWidget::HandlePlay2() { PlayScene(2); }
void UCallSheetMenuWidget::HandlePlay3() { PlayScene(3); }
void UCallSheetMenuWidget::HandlePlay4() { PlayScene(4); }

void UCallSheetMenuWidget::HandleVolumeChanged(float)
{
	URetakeAudioSubsystem::SetVolumes(this, MusicSlider->GetValue(), SfxSlider->GetValue(), VoiceSlider->GetValue(), true);
}

void UCallSheetMenuWidget::HandleQuit()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

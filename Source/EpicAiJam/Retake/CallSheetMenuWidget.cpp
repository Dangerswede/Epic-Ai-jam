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
		Cell(C, 170.f);
		return H;
	};
	Box->AddChildToVerticalBox(Row(TEXT("SCENE"), TEXT("NOTES"), TEXT("STATUS"), 14, Red()));

	const TArray<RetakeScenes::FSceneInfo>& Scenes = RetakeScenes::All();
	for (int32 i = 0; i < Scenes.Num() && i < 3; ++i)
	{
		const bool bUnlocked = Save && Save->IsUnlocked(i);
		const int32* Best = Save ? Save->BestTakes.Find(Scenes[i].MapName) : nullptr;
		FString Status = !bUnlocked ? TEXT("LOCKED") : Best ? FString::Printf(TEXT("WRAPPED (%d takes)"), *Best) : TEXT("READY");

		UHorizontalBox* H = Row(Scenes[i].Title, Scenes[i].Logline, Status, 18, bUnlocked ? Ink() : Dim());
		UButton* Play = Button(WidgetTree, bUnlocked ? TEXT("PLAY") : TEXT("---"), 16);
		Play->SetIsEnabled(bUnlocked);
		H->AddChildToHorizontalBox(Play)->SetVerticalAlignment(VAlign_Center);
		// AddDynamic needs a literal function name.
		switch (i)
		{
		case 0: Play->OnClicked.AddDynamic(this, &UCallSheetMenuWidget::HandlePlay0); break;
		case 1: Play->OnClicked.AddDynamic(this, &UCallSheetMenuWidget::HandlePlay1); break;
		default: Play->OnClicked.AddDynamic(this, &UCallSheetMenuWidget::HandlePlay2); break;
		}
		Box->AddChildToVerticalBox(H)->SetPadding(FMargin(0.f, 8.f));
	}

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

void UCallSheetMenuWidget::HandleQuit()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

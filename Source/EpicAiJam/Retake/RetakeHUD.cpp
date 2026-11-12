#include "Retake/RetakeHUD.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Retake/CallSheetMenuWidget.h"
#include "Retake/RetakeHUDWidget.h"
#include "Retake/RetakeResultsWidget.h"
#include "TimerManager.h"

ARetakeHUD::ARetakeHUD()
{
	HUDWidgetClass = URetakeHUDWidget::StaticClass();
	ResultsWidgetClass = URetakeResultsWidget::StaticClass();
}

void ARetakeHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	HUDWidget = CreateWidget<URetakeHUDWidget>(PC, HUDWidgetClass ? HUDWidgetClass.Get() : URetakeHUDWidget::StaticClass());
	HUDWidget->AddToViewport(0);

	ResultsWidget = CreateWidget<URetakeResultsWidget>(PC, ResultsWidgetClass ? ResultsWidgetClass.Get() : URetakeResultsWidget::StaticClass());
	ResultsWidget->AddToViewport(10);
	ResultsWidget->SetVisibility(ESlateVisibility::Collapsed);

	Director = ARetakeSceneDirector::Get(this);
	if (ARetakeSceneDirector* D = Director.Get())
	{
		HUDWidget->SetDirector(D);
		D->OnStateChanged.AddDynamic(this, &ARetakeHUD::HandleStateChanged);
	}
	else
	{
		HUDWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ARetakeHUD::HandleStateChanged(ERetakeSceneState NewState, ERetakeSceneState OldState)
{
	ARetakeSceneDirector* D = Director.Get();
	if (!D || !ResultsWidget)
	{
		return;
	}
	if (NewState == ERetakeSceneState::Results)
	{
		ResultsWidget->ShowResults(D, D->WasLastWrapNewBest());
	}
	else if (ResultsWidget->GetVisibility() != ESlateVisibility::Collapsed)
	{
		ResultsWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

ARetakeMenuHUD::ARetakeMenuHUD()
{
	MenuWidgetClass = UCallSheetMenuWidget::StaticClass();
}

void ARetakeMenuHUD::BeginPlay()
{
	Super::BeginPlay();
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}
	UCallSheetMenuWidget* Menu = CreateWidget<UCallSheetMenuWidget>(PC, MenuWidgetClass ? MenuWidgetClass.Get() : UCallSheetMenuWidget::StaticClass());
	Menu->AddToViewport();
	PC->SetShowMouseCursor(true);
	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(Menu->TakeWidget());
	PC->SetInputMode(Mode);
}

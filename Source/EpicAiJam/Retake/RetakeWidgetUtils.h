#pragma once

#include "CoreMinimal.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Fonts/SlateFontInfo.h"

/**
 * Small helpers to build UMG widget trees in C++ (no widget blueprints needed).
 * (Godot: building Control nodes in code with add_child instead of a .tscn.)
 */
namespace RetakeUI
{
	// Call-sheet palette.
	inline FLinearColor Paper()   { return FLinearColor(0.93f, 0.89f, 0.78f, 1.f); }
	inline FLinearColor Ink()     { return FLinearColor(0.05f, 0.05f, 0.05f, 1.f); }
	inline FLinearColor Slate()   { return FLinearColor(0.08f, 0.08f, 0.09f, 0.92f); }
	inline FLinearColor Chalk()   { return FLinearColor(0.95f, 0.95f, 0.92f, 1.f); }
	inline FLinearColor Red()     { return FLinearColor(0.85f, 0.08f, 0.06f, 1.f); }
	inline FLinearColor Gold()    { return FLinearColor(1.f, 0.78f, 0.15f, 1.f); }
	inline FLinearColor Dim()     { return FLinearColor(0.45f, 0.45f, 0.45f, 1.f); }

	inline UTextBlock* Text(UWidgetTree* Tree, const FString& Str, int32 Size, const FLinearColor& Color,
		ETextJustify::Type Justify = ETextJustify::Left, bool bBold = true)
	{
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		T->SetText(FText::FromString(Str));
		FSlateFontInfo Font = T->GetFont();
		Font.Size = Size;
		Font.TypefaceFontName = bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular"));
		T->SetFont(Font);
		T->SetColorAndOpacity(FSlateColor(Color));
		T->SetJustification(Justify);
		T->SetAutoWrapText(true);
		return T;
	}

	inline UBorder* Panel(UWidgetTree* Tree, const FLinearColor& Color, const FMargin& Padding)
	{
		UBorder* B = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		B->SetBrushColor(Color);
		B->SetPadding(Padding);
		return B;
	}

	/** Button with a text label. Returns the button; label via OutLabel if wanted. */
	inline UButton* Button(UWidgetTree* Tree, const FString& Label, int32 Size, UTextBlock** OutLabel = nullptr)
	{
		UButton* Btn = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		Btn->SetBackgroundColor(FLinearColor(0.15f, 0.15f, 0.15f, 1.f));
		UTextBlock* T = Text(Tree, Label, Size, Chalk(), ETextJustify::Center);
		Btn->AddChild(T);
		if (OutLabel)
		{
			*OutLabel = T;
		}
		return Btn;
	}
}

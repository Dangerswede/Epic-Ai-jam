#include "Retake/RetakeWidgetUtils.h"

#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Fonts/CompositeFont.h"
#include "Retake/RetakeAudioSet.h"

namespace
{
	/** Composite font built once from the configured font face (keeps the face alive while in use). */
	TSharedPtr<const FCompositeFont> GetPopComposite()
	{
		static TSharedPtr<const FCompositeFont> Cached;
		static TStrongObjectPtr<UFontFace> Face;
		if (!Cached.IsValid())
		{
			const URetakeSettings* S = GetDefault<URetakeSettings>();
			if (UFontFace* Loaded = S ? S->PopFontFace.LoadSynchronous() : nullptr)
			{
				Face.Reset(Loaded);
				TSharedRef<FCompositeFont> Composite = MakeShared<FStandaloneCompositeFont>();
				FTypefaceEntry& Entry = Composite->DefaultTypeface.Fonts.Emplace_GetRef(TEXT("Regular"));
				Entry.Font = FFontData(Loaded);
				Cached = Composite;
			}
		}
		return Cached;
	}
}

FSlateFontInfo RetakeUI::PopFont(int32 Size)
{
	FSlateFontInfo Font;
	if (TSharedPtr<const FCompositeFont> Composite = GetPopComposite())
	{
		Font = FSlateFontInfo(Composite, Size, TEXT("Regular"));
	}
	else
	{
		Font = FCoreStyle::GetDefaultFontStyle("Bold", Size);
	}
	Font.OutlineSettings.OutlineSize = FMath::Max(2, Size / 14);
	Font.OutlineSettings.OutlineColor = Brown();
	return Font;
}

UTextBlock* RetakeUI::PopText(UWidgetTree* Tree, const FString& Str, int32 Size, const FLinearColor& Color)
{
	UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	T->SetText(FText::FromString(Str));
	T->SetFont(PopFont(Size));
	T->SetColorAndOpacity(FSlateColor(Color));
	T->SetShadowOffset(FVector2D(FMath::Max(2, Size / 12), FMath::Max(2, Size / 12)));
	T->SetShadowColorAndOpacity(FLinearColor(0.1f, 0.03f, 0.01f, 0.85f));
	T->SetJustification(ETextJustify::Center);
	T->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	return T;
}

float RetakeUI::PopScale(float T)
{
	if (T <= 0.f) return 0.f;
	if (T < 0.12f)
	{
		const float A = T / 0.12f;
		return 1.3f * (1.f - FMath::Square(1.f - A)); // ease out to the overshoot
	}
	if (T < 0.26f)
	{
		return FMath::Lerp(1.3f, 1.f, FMath::SmoothStep(0.f, 1.f, (T - 0.12f) / 0.14f));
	}
	return 1.f;
}

float RetakeUI::PopTilt(float T, float BaseAngle)
{
	return BaseAngle + 9.f * FMath::Sin(T * 24.f) * FMath::Exp(-T * 9.f);
}

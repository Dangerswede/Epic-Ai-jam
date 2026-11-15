#include "Retake/RetakeButton.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Retake/RetakeAudioSubsystem.h"
#include "Retake/RetakeWidgetUtils.h"

namespace
{
	constexpr float HoverScale = 1.07f;
	constexpr float PressScale = 0.92f;
	constexpr float HoverAngle = -1.8f;
	constexpr float Stiffness = 650.f;
	constexpr float Damping = 22.f; // under-damped: a small bounce

	/** Semi-implicit spring step; sub-stepped so long frames stay stable. */
	void SpringTo(float& X, float& V, float Target, float Dt)
	{
		float Left = FMath::Min(Dt, 0.1f);
		while (Left > 0.f)
		{
			const float H = FMath::Min(Left, 1.f / 120.f);
			V += (Stiffness * (Target - X) - Damping * V) * H;
			X += V * H;
			Left -= H;
		}
	}
}

URetakeButton::URetakeButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
}

TSharedRef<SWidget> URetakeButton::RebuildWidget()
{
	OnHovered.AddUniqueDynamic(this, &URetakeButton::FxHovered);
	OnUnhovered.AddUniqueDynamic(this, &URetakeButton::FxUnhovered);
	OnPressed.AddUniqueDynamic(this, &URetakeButton::FxPressed);
	OnReleased.AddUniqueDynamic(this, &URetakeButton::FxReleased);
	OnClicked.AddUniqueDynamic(this, &URetakeButton::FxClicked);
	return Super::RebuildWidget();
}

void URetakeButton::ReleaseSlateResources(bool bReleaseChildren)
{
	if (TickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
		TickHandle.Reset();
	}
	Super::ReleaseSlateResources(bReleaseChildren);
}

void URetakeButton::BeginDestroy()
{
	if (TickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
		TickHandle.Reset();
	}
	Super::BeginDestroy();
}

void URetakeButton::PlayIntro(float Delay)
{
	IntroTime = -FMath::Max(0.f, Delay);
	ApplyVisuals();
	Wake();
}

void URetakeButton::FxHovered()
{
	bHovered = true;
	AngleVel -= 40.f; // little kick so the tilt wobbles in
	if (UWorld* World = GetWorld())
	{
		if (URetakeAudioSubsystem* Audio = World->GetSubsystem<URetakeAudioSubsystem>())
		{
			Audio->PlayUIHover();
		}
	}
	Wake();
}

void URetakeButton::FxUnhovered()
{
	bHovered = false;
	bPressed = false;
	Wake();
}

void URetakeButton::FxPressed()
{
	bPressed = true;
	ScaleVel -= 3.f; // snap down
	Wake();
}

void URetakeButton::FxReleased()
{
	bPressed = false;
	ScaleVel += 2.f; // spring back past the hover size
	Wake();
}

void URetakeButton::FxClicked()
{
	if (UWorld* World = GetWorld())
	{
		if (URetakeAudioSubsystem* Audio = World->GetSubsystem<URetakeAudioSubsystem>())
		{
			Audio->PlayUIClick();
		}
	}
}

void URetakeButton::Wake()
{
	if (!TickHandle.IsValid())
	{
		TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &URetakeButton::TickFx));
	}
}

bool URetakeButton::TickFx(float DeltaTime)
{
	const float Dt = FMath::Min(DeltaTime, 0.1f);
	const float ScaleTarget = bPressed ? PressScale : (bHovered ? HoverScale : 1.f);
	const float AngleTarget = bHovered && !bPressed ? HoverAngle : 0.f;
	SpringTo(Scale, ScaleVel, ScaleTarget, Dt);
	SpringTo(Angle, AngleVel, AngleTarget, Dt);
	Hover01 = FMath::FInterpTo(Hover01, bHovered ? 1.f : 0.f, Dt, 16.f);
	Press01 = FMath::FInterpTo(Press01, bPressed ? 1.f : 0.f, Dt, 30.f);
	if (IntroTime < 1.f)
	{
		IntroTime += Dt;
	}
	ApplyVisuals();

	const bool bSettled = IntroTime >= 1.f
		&& FMath::Abs(Scale - ScaleTarget) < 0.001f && FMath::Abs(ScaleVel) < 0.01f
		&& FMath::Abs(Angle - AngleTarget) < 0.01f && FMath::Abs(AngleVel) < 0.05f
		&& FMath::Abs(Hover01 - (bHovered ? 1.f : 0.f)) < 0.005f
		&& FMath::Abs(Press01 - (bPressed ? 1.f : 0.f)) < 0.005f;
	if (bSettled)
	{
		Scale = ScaleTarget;
		ScaleVel = 0.f;
		Angle = AngleTarget;
		AngleVel = 0.f;
		Hover01 = bHovered ? 1.f : 0.f;
		Press01 = bPressed ? 1.f : 0.f;
		ApplyVisuals();
		TickHandle.Reset();
		return false; // stop ticking until the next hover/press
	}
	return true;
}

void URetakeButton::ApplyVisuals()
{
	float IntroScale = 1.f;
	float Opacity = 1.f;
	if (IntroTime < 0.f)
	{
		IntroScale = 0.f;
		Opacity = 0.f;
	}
	else if (IntroTime < 1.f)
	{
		IntroScale = RetakeUI::PopScale(IntroTime);
		Opacity = FMath::Clamp(IntroTime / 0.08f, 0.f, 1.f);
	}
	SetRenderTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(IntroScale * Scale), FVector2D::ZeroVector, Angle));
	SetRenderOpacity(Opacity);
	const FLinearColor Bg = FMath::Lerp(FMath::Lerp(IdleColor, HoverColor, Hover01), PressColor, Press01);
	SetBackgroundColor(Bg);
}

// ------------------------------------------------------------------------------------------------

void RetakeUI::PlayButtonIntros(UWidgetTree* Tree, float StartDelay, float Stagger, int32 MaxStaggered)
{
	if (!Tree)
	{
		return;
	}
	int32 Index = 0;
	Tree->ForEachWidget([&](UWidget* W)
	{
		if (URetakeButton* B = Cast<URetakeButton>(W))
		{
			B->PlayIntro(StartDelay + Stagger * FMath::Min(Index, MaxStaggered));
			++Index;
		}
	});
}

void RetakeUI::PopIn(UWidget* Widget, float Strength, float Delay)
{
	if (!Widget)
	{
		return;
	}
	Widget->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	Widget->SetRenderScale(FVector2D(1.f - Strength));
	Widget->SetRenderOpacity(0.f);
	TWeakObjectPtr<UWidget> Weak(Widget);
	float T = -FMath::Max(0.f, Delay);
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak, Strength, T](float DeltaTime) mutable
	{
		UWidget* W = Weak.Get();
		if (!W)
		{
			return false;
		}
		T += FMath::Min(DeltaTime, 0.1f);
		if (T < 0.f)
		{
			return true;
		}
		// PopScale runs 0 -> 1.3 -> 1; blend it toward 1 so a whole panel only nudges.
		const float S = FMath::Lerp(1.f - Strength, 1.f, FMath::Clamp(T / 0.12f, 0.f, 1.f))
			+ Strength * FMath::Max(0.f, RetakeUI::PopScale(T) - 1.f);
		W->SetRenderScale(FVector2D(S));
		W->SetRenderOpacity(FMath::Clamp(T / 0.12f, 0.f, 1.f));
		if (T >= 0.3f)
		{
			W->SetRenderScale(FVector2D(1.f));
			W->SetRenderOpacity(1.f);
			return false;
		}
		return true;
	}));
}

#include "Retake/LightFlicker.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/LightComponent.h"
#include "EngineUtils.h"
#include "Retake/RetakeSceneDirector.h"

ALightFlicker::ALightFlicker()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ALightFlicker::BeginPlay()
{
	Super::BeginPlay();
	// Every non-directional light in the level (the studio lights).
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		TArray<ULightComponent*> Comps;
		It->GetComponents(Comps);
		for (ULightComponent* L : Comps)
		{
			if (L && !L->IsA<UDirectionalLightComponent>())
			{
				Lights.Add(L);
				BaseIntensity.Add(L->Intensity);
			}
		}
	}
	// The director may begin play after us; bind next tick.
	GetWorldTimerManager().SetTimerForNextTick([this]()
	{
		if (ARetakeSceneDirector* D = ARetakeSceneDirector::Get(this))
		{
			D->OnTakeCut.AddDynamic(this, &ALightFlicker::HandleTakeCut);
		}
	});
}

void ALightFlicker::HandleTakeCut(int32 TakeNumber, ERetakeCutReason Reason)
{
	TimeLeft = FlickerSeconds;
	StepClock = 0.f;
}

void ALightFlicker::Restore()
{
	for (int32 i = 0; i < Lights.Num(); ++i)
	{
		if (ULightComponent* L = Lights[i].Get())
		{
			L->SetIntensity(BaseIntensity[i]);
		}
	}
}

void ALightFlicker::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (TimeLeft <= 0.f)
	{
		return;
	}
	TimeLeft -= DeltaSeconds;
	StepClock -= DeltaSeconds;
	if (TimeLeft <= 0.f)
	{
		Restore();
		return;
	}
	if (StepClock <= 0.f)
	{
		StepClock = FMath::FRandRange(0.04f, 0.1f);
		for (int32 i = 0; i < Lights.Num(); ++i)
		{
			if (ULightComponent* L = Lights[i].Get())
			{
				// Mostly dips, the odd blackout.
				const float Scale = FMath::FRand() < 0.25f ? 0.05f : FMath::FRandRange(0.3f, 1.f);
				L->SetIntensity(BaseIntensity[i] * Scale);
			}
		}
	}
}

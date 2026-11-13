#include "Retake/SteamWisp.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Retake/RetakeActivator.h"
#include "UObject/ConstructorHelpers.h"

ASteamWisp::ASteamWisp()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Glow(TEXT("/Game/Retake/FX/M_FxGlow.M_FxGlow"));
	for (int32 i = 0; i < 4; ++i)
	{
		UStaticMeshComponent* Puff = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Puff%d"), i));
		Puff->SetupAttachment(Root);
		Puff->SetStaticMesh(RetakeVisuals::LoadBasicShape(TEXT("Sphere")));
		Puff->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Puff->SetCastShadow(false);
		Puff->SetCanEverAffectNavigation(false);
		if (Glow.Succeeded())
		{
			Puff->SetMaterial(0, Glow.Object);
		}
		Puffs.Add(Puff);
	}
}

void ASteamWisp::BeginPlay()
{
	Super::BeginPlay();
	for (UStaticMeshComponent* Puff : Puffs)
	{
		RetakeVisuals::Tint(Puff, FLinearColor(1.f, 0.97f, 0.92f) * 2.5f); // the scene runs on physical light units
	}
}

void ASteamWisp::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Clock += DeltaSeconds;
	for (int32 i = 0; i < Puffs.Num(); ++i)
	{
		// Each puff is a quarter cycle behind the previous one.
		const float T = FMath::Fmod(Clock / CycleSeconds + i / float(Puffs.Num()), 1.f);
		const float Sway = FMath::Sin((Clock + i) * 2.3f) * 3.f * T;
		Puffs[i]->SetRelativeLocation(FVector(Sway, Sway * 0.5f, T * RiseHeight));
		Puffs[i]->SetRelativeScale3D(FVector(FMath::Lerp(0.06f, 0.2f, T)));
		if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Puffs[i]->GetMaterial(0)))
		{
			// Fade in quickly, out slowly; faint enough to read as steam, not as a light.
			const float Alpha = FMath::Min(T / 0.15f, 1.f) * (1.f - T);
			MID->SetScalarParameterValue(TEXT("Intensity"), 0.25f * Alpha);
		}
	}
}

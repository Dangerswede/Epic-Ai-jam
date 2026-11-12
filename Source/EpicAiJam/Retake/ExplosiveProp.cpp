#include "Retake/ExplosiveProp.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Retake/RetakeActivator.h"
#include "Retake/RetakeCharacter.h"
#include "Retake/RetakeSceneDirector.h"

AExplosiveProp::AExplosiveProp()
{
	PrimaryActorTick.bCanEverTick = true;

	BarrelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrelMesh"));
	SetRootComponent(BarrelMesh);
	BarrelMesh->SetStaticMesh(RetakeVisuals::LoadBasicShape(TEXT("Cylinder")));
	BarrelMesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.9f));
	BarrelMesh->SetCollisionProfileName(TEXT("BlockAll"));
	BarrelMesh->SetMobility(EComponentMobility::Movable);

	FlashMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlashMesh"));
	FlashMesh->SetupAttachment(BarrelMesh);
	FlashMesh->SetStaticMesh(RetakeVisuals::LoadBasicShape(TEXT("Sphere")));
	FlashMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FlashMesh->SetCastShadow(false);
	FlashMesh->SetVisibility(false);
	FlashMesh->SetMobility(EComponentMobility::Movable);

	FuseLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("FuseLabel"));
	FuseLabel->SetupAttachment(BarrelMesh);
	FuseLabel->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
	FuseLabel->SetHorizontalAlignment(EHTA_Center);
	FuseLabel->SetWorldSize(40.f);
	FuseLabel->SetTextRenderColor(FColor(255, 200, 0));
	FuseLabel->SetText(FText::FromString(TEXT("[E] ARM")));
}

void AExplosiveProp::BeginPlay()
{
	Super::BeginPlay();
	RetakeVisuals::Tint(BarrelMesh, FLinearColor(0.8f, 0.1f, 0.05f));
	RetakeVisuals::Tint(FlashMesh, FLinearColor(5.f, 3.f, 0.5f));
}

void AExplosiveProp::RetakeInteract(AActor* Interactor)
{
	if (bArmed || bExploded)
	{
		return;
	}
	bArmed = true;
	FuseLeft = FuseSeconds;
	OnArmed.Broadcast(this);
}

void AExplosiveProp::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (FlashLeft > 0.f)
	{
		FlashLeft -= DeltaSeconds;
		const float Alpha = 1.f - FMath::Clamp(FlashLeft / 0.35f, 0.f, 1.f);
		// Sphere is 100 cm; grow to the blast diameter.
		FlashMesh->SetWorldScale3D(FVector(FMath::Lerp(0.5f, Radius / 50.f, Alpha)));
		if (FlashLeft <= 0.f)
		{
			FlashMesh->SetVisibility(false);
		}
	}

	if (!bArmed || bExploded)
	{
		return;
	}
	FuseLeft -= DeltaSeconds;
	FuseLabel->SetText(FText::FromString(FString::Printf(TEXT("%d"), FMath::Max(1, FMath::CeilToInt(FuseLeft)))));
	// Blink faster as the fuse runs down.
	const float BlinkRate = FMath::Lerp(8.f, 2.f, FMath::Clamp(FuseLeft / FuseSeconds, 0.f, 1.f));
	const bool bBlinkOn = FMath::Fmod(FuseLeft * BlinkRate, 1.f) > 0.5f;
	RetakeVisuals::Tint(BarrelMesh, bBlinkOn ? FLinearColor(1.f, 0.9f, 0.1f) : FLinearColor(0.8f, 0.1f, 0.05f));
	if (FuseLeft <= 0.f)
	{
		Explode();
	}
}

void AExplosiveProp::Explode()
{
	bExploded = true;
	bArmed = false;
	const FVector Center = GetActorLocation();

	// Break tagged walls.
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor != this && Actor->ActorHasTag(BreakableTag) && !Actor->IsHidden() &&
			FVector::Dist(Actor->GetActorLocation(), Center) <= Radius + Actor->GetSimpleCollisionRadius())
		{
			Actor->SetActorHiddenInGame(true);
			Actor->SetActorEnableCollision(false);
			BrokenActors.Add(Actor);
		}
	}

	// Player in radius -> CUT. Ghosts are unaffected.
	for (TActorIterator<ARetakeCharacter> It(GetWorld()); It; ++It)
	{
		if (It->GetActorEnableCollision() && FVector::Dist(It->GetActorLocation(), Center) <= Radius)
		{
			if (ARetakeSceneDirector* Director = ARetakeSceneDirector::Get(this))
			{
				Director->NotifyPlayerKilled();
			}
		}
	}

	if (ExplosionEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ExplosionEffect, Center);
	}
	FlashMesh->SetVisibility(true);
	FlashLeft = 0.35f;

	BarrelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RetakeVisuals::Tint(BarrelMesh, FLinearColor(0.05f, 0.05f, 0.05f));
	FuseLabel->SetText(FText::GetEmpty());

	OnExploded.Broadcast(this);
}

void AExplosiveProp::ResetForTake()
{
	for (AActor* Actor : BrokenActors)
	{
		if (IsValid(Actor))
		{
			Actor->SetActorHiddenInGame(false);
			Actor->SetActorEnableCollision(true);
		}
	}
	BrokenActors.Reset();
	bArmed = false;
	bExploded = false;
	FuseLeft = 0.f;
	FlashLeft = 0.f;
	FlashMesh->SetVisibility(false);
	BarrelMesh->SetCollisionProfileName(TEXT("BlockAll"));
	RetakeVisuals::Tint(BarrelMesh, FLinearColor(0.8f, 0.1f, 0.05f));
	FuseLabel->SetText(FText::FromString(TEXT("[E] ARM")));
}

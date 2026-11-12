#include "Retake/ExplosiveProp.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Retake/RetakeHUD.h"
#include "Retake/RetakeHUDWidget.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
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

	ArtMesh = RetakeVisuals::CreateArtComponent(this, BarrelMesh);

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

	FlashLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FlashLight"));
	FlashLight->SetupAttachment(BarrelMesh);
	FlashLight->SetUsingAbsoluteScale(true);
	FlashLight->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	FlashLight->SetIntensity(0.f);
	FlashLight->SetLightColor(FLinearColor(1.f, 0.6f, 0.25f));
	FlashLight->SetCastShadows(false);
	FlashLight->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowMat(TEXT("/Game/Retake/FX/M_FxGlow.M_FxGlow"));
	if (GlowMat.Succeeded())
	{
		FlashMesh->SetMaterial(0, GlowMat.Object); // fireball core: additive glow, faded by "Intensity"
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> DefaultFx(TEXT("/Game/Retake/FX/NS_Explosion.NS_Explosion"));
	if (DefaultFx.Succeeded())
	{
		ExplosionEffect = DefaultFx.Object;
	}
}

void AExplosiveProp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RetakeVisuals::ApplyArt(BarrelMesh, ArtMesh, ArtMeshAsset, ArtSize);
}

void AExplosiveProp::BeginPlay()
{
	Super::BeginPlay();
	RetakeVisuals::Tint(BarrelMesh, FLinearColor(0.8f, 0.1f, 0.05f));
	RetakeVisuals::Tint(FlashMesh, FLinearColor(9.f, 3.2f, 0.6f) * GlowBoost);
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
		const float Alpha = 1.f - FMath::Clamp(FlashLeft / 0.45f, 0.f, 1.f);
		// Fireball: the 100 cm sphere balloons out fast (ease-out) to ~90% of the blast radius and burns out.
		const float Grow = 1.f - FMath::Square(1.f - Alpha);
		FlashMesh->SetWorldScale3D(FVector(FMath::Lerp(0.4f, Radius * 0.9f / 50.f, Grow)));
		if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(FlashMesh->GetMaterial(0)))
		{
			MID->SetScalarParameterValue(TEXT("Intensity"), FMath::Square(1.f - Alpha) * 1.5f);
		}
		FlashLight->SetIntensity(FMath::Square(1.f - Alpha) * 120000.f);
		FlashLight->SetAttenuationRadius(Radius * 3.f);
		if (FlashLeft <= 0.f)
		{
			FlashMesh->SetVisibility(false);
			FlashLight->SetVisibility(false);
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
			ThrowDebris(Actor, Center);
		}
	}

	// Player in radius -> CUT. Ghosts are unaffected.
	for (TActorIterator<ARetakeCharacter> It(GetWorld()); It; ++It)
	{
		if (It->GetActorEnableCollision() && FVector::Dist(It->GetActorLocation(), Center) <= Radius)
		{
			if (ARetakeSceneDirector* Director = ARetakeSceneDirector::Get(this))
			{
				Director->BeginStuntDeath(ERetakeCutReason::Killed, Center);
			}
		}
	}

	if (ExplosionEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ExplosionEffect, Center + FVector(0.f, 0.f, 40.f), FRotator::ZeroRotator,
			FVector(EffectScalePer100 * Radius / 100.f));
	}
	FlashMesh->SetVisibility(true);
	FlashLight->SetVisibility(true);
	FlashLeft = 0.45f;
	AddPunch(Center);

	BarrelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ArtMesh->SetVisibility(false);
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
	for (AActor* Chunk : Debris)
	{
		if (IsValid(Chunk))
		{
			Chunk->Destroy();
		}
	}
	Debris.Reset();
	FlashLight->SetVisibility(false);
	bArmed = false;
	bExploded = false;
	FuseLeft = 0.f;
	FlashLeft = 0.f;
	FlashMesh->SetVisibility(false);
	ArtMesh->SetVisibility(true);
	BarrelMesh->SetCollisionProfileName(TEXT("BlockAll"));
	RetakeVisuals::Tint(BarrelMesh, FLinearColor(0.8f, 0.1f, 0.05f));
	FuseLabel->SetText(FText::FromString(TEXT("[E] ARM")));
}

void AExplosiveProp::AddPunch(const FVector& Center)
{
	UWorld* World = GetWorld();
	const ARetakeSceneDirector* Director = ARetakeSceneDirector::Get(this);
	const bool bRolling = Director && Director->GetState() == ERetakeSceneState::Rolling;

	// Shove anything that simulates physics (debris, loose props).
	TArray<FOverlapResult> Hits;
	World->OverlapMultiByObjectType(Hits, Center, FQuat::Identity, FCollisionObjectQueryParams(ECC_PhysicsBody),
		FCollisionShape::MakeSphere(Radius * 1.5f));
	for (const FOverlapResult& Hit : Hits)
	{
		if (UPrimitiveComponent* Prim = Hit.GetComponent(); Prim && Prim->IsSimulatingPhysics())
		{
			Prim->AddRadialImpulse(Center, Radius * 1.5f, ImpulseStrength, ERadialImpulseFalloff::RIF_Linear, true);
		}
	}

	// Screen flash + camera shake, scaled by how close the player's camera is.
	APlayerController* PC = World->GetFirstPlayerController();
	if (PC && PC->PlayerCameraManager)
	{
		const float Dist = FVector::Dist(PC->PlayerCameraManager->GetCameraLocation(), Center);
		const float Feel = 1.f - FMath::Clamp(Dist / (Radius * FeelRangeMultiplier), 0.f, 1.f);
		if (Feel > 0.f)
		{
			if (const ARetakeHUD* Hud = PC->GetHUD<ARetakeHUD>(); Hud && Hud->GetHUDWidget())
			{
				Hud->GetHUDWidget()->FlashScreen(0.35f + 0.6f * Feel);
			}
			if (ARetakeCharacter* Player = Cast<ARetakeCharacter>(PC->GetPawn()); Player && bRolling)
			{
				Player->PlayStuntShake(0.5f, 6.f + 18.f * Feel);
			}
		}
	}

	// Hit-stop: a short global slow-mo while a take is rolling (the Director's Cut runs its own slow-mo).
	if (bRolling && SlowMoDilation < 1.f && UGameplayStatics::GetGlobalTimeDilation(this) >= 0.99f)
	{
		UGameplayStatics::SetGlobalTimeDilation(this, SlowMoDilation);
		FTimerHandle Handle;
		World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			const ARetakeSceneDirector* D = ARetakeSceneDirector::Get(this);
			if (!D || D->GetState() != ERetakeSceneState::DirectorsCut)
			{
				UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
			}
		}), SlowMoSeconds * SlowMoDilation, false);
	}
}

void AExplosiveProp::ThrowDebris(const AActor* Wall, const FVector& Center)
{
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube || DebrisPerWall <= 0)
	{
		return;
	}
	FVector Origin, Extent;
	Wall->GetActorBounds(true, Origin, Extent);
	FRandomStream Rng(GetTypeHash(Wall->GetFName()));
	for (int32 i = 0; i < DebrisPerWall; ++i)
	{
		const FVector At = Origin + FVector(Rng.FRandRange(-Extent.X, Extent.X), Rng.FRandRange(-Extent.Y, Extent.Y), Rng.FRandRange(-Extent.Z, Extent.Z) * 0.8f);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Chunk = GetWorld()->SpawnActor<AStaticMeshActor>(At, FRotator(Rng.FRandRange(0.f, 360.f), Rng.FRandRange(0.f, 360.f), 0.f), Params);
		if (!Chunk)
		{
			continue;
		}
		UStaticMeshComponent* Mesh = Chunk->GetStaticMeshComponent();
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetStaticMesh(Cube);
		Mesh->SetWorldScale3D(FVector(Rng.FRandRange(0.15f, 0.4f), Rng.FRandRange(0.15f, 0.35f), Rng.FRandRange(0.12f, 0.3f)));
		Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
		Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);   // never blocks a stunt double or a guard
		Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->SetSimulatePhysics(true);
		RetakeVisuals::Tint(Mesh, FLinearColor(0.35f, 0.3f, 0.26f));
		const FVector Away = (At - Center).GetSafeNormal() + FVector(0.f, 0.f, 0.6f);
		Mesh->AddImpulse(Away.GetSafeNormal() * ImpulseStrength * Rng.FRandRange(0.6f, 1.2f), NAME_None, true);
		Mesh->AddAngularImpulseInDegrees(FVector(Rng.FRandRange(-600.f, 600.f), Rng.FRandRange(-600.f, 600.f), 0.f), NAME_None, true);
		Chunk->SetLifeSpan(6.f);
		Debris.Add(Chunk);
	}
}
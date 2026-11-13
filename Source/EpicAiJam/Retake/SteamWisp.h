#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SteamWisp.generated.h"

class UStaticMeshComponent;

/**
 * Set dressing only (story, scene 5): a little steam rising from a coffee cup nobody is drinking.
 * A few faint additive puffs drift up, grow and fade, on a loop. No collision, no gameplay.
 * (Godot: a tiny GPUParticles3D with a slow upward velocity.)
 */
UCLASS()
class EPICAIJAM_API ASteamWisp : public AActor
{
	GENERATED_BODY()

public:
	ASteamWisp();
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Steam")
	float RiseHeight = 35.f;

	UPROPERTY(EditAnywhere, Category = "Steam")
	float CycleSeconds = 2.6f;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UStaticMeshComponent>> Puffs;
	float Clock = 0.f;
};

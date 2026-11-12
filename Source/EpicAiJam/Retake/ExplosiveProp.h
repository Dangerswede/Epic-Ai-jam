#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Retake/RetakeInteractable.h"
#include "Retake/RetakeResettable.h"
#include "ExplosiveProp.generated.h"

class UNiagaraSystem;
class UStaticMeshComponent;
class UTextRenderComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FExplosivePropSignature, AExplosiveProp*, Prop);

/**
 * Stunt explosive. Interact (player or ghost) arms it; it explodes after FuseSeconds.
 * Breaks actors tagged BreakableTag in Radius (hidden + no collision, restored on reset)
 * and cuts the take if the player is in Radius. Ghosts are unaffected.
 * (Godot: an interactable node with a Timer that emits "exploded".)
 */
UCLASS()
class EPICAIJAM_API AExplosiveProp : public AActor, public IRetakeInteractable, public IRetakeResettable
{
	GENERATED_BODY()

public:
	AExplosiveProp();

	virtual void Tick(float DeltaSeconds) override;
	virtual void RetakeInteract(AActor* Interactor) override;
	virtual void ResetForTake() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Art pass: optional prop mesh shown instead of the graybox (which stays as the collision). Pivot bottom-centre. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art")
	TObjectPtr<UStaticMesh> ArtMeshAsset;

	/** Size (cm) the art mesh is scaled to. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art")
	FVector ArtSize = FVector(55.f, 55.f, 90.f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Art")
	TObjectPtr<UStaticMeshComponent> ArtMesh;


	UPROPERTY(BlueprintAssignable, Category = "Retake")
	FExplosivePropSignature OnArmed;

	/** For audio / camera shake. */
	UPROPERTY(BlueprintAssignable, Category = "Retake")
	FExplosivePropSignature OnExploded;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retake")
	float FuseSeconds = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retake")
	float Radius = 400.f;

	/** Actors with this tag inside Radius get broken. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retake")
	FName BreakableTag = TEXT("BreakableWall");

	/** Niagara burst (fireball, smoke, sparks). Defaults to /Game/Retake/FX/NS_Explosion. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retake|Punch")
	TObjectPtr<UNiagaraSystem> ExplosionEffect;

	/** Niagara scale per 100 cm of Radius. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retake|Punch")
	float EffectScalePer100 = 1.6f;

	/** Brightness of the fireball core (emissive multiplier, the scene runs on physical light units). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retake|Punch")
	float GlowBoost = 40.f;

	/** Global slow-mo when it goes off during a take (real seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retake|Punch")
	float SlowMoDilation = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retake|Punch")
	float SlowMoSeconds = 0.3f;

	/** Camera shake and screen flash reach (multiples of Radius). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retake|Punch")
	float FeelRangeMultiplier = 4.f;

	/** Chunks thrown from each broken wall (physics, no collision with pawns). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retake|Punch")
	int32 DebrisPerWall = 12;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retake|Punch")
	float ImpulseStrength = 900.f;

	UFUNCTION(BlueprintPure, Category = "Retake")
	bool IsArmed() const { return bArmed; }

	UFUNCTION(BlueprintPure, Category = "Retake")
	bool HasExploded() const { return bExploded; }

protected:
	virtual void BeginPlay() override;

	/** Swap for art later. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<UStaticMeshComponent> BarrelMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<UStaticMeshComponent> FlashMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<class UPointLightComponent> FlashLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<UTextRenderComponent> FuseLabel;

private:
	void Explode();
	void AddPunch(const FVector& Center);
	void ThrowDebris(const AActor* Wall, const FVector& Center);

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> BrokenActors;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> Debris;

	float FuseLeft = 0.f;
	float FlashLeft = 0.f;
	bool bArmed = false;
	bool bExploded = false;
};

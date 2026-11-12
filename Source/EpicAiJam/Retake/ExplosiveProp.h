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

	/** Optional Niagara burst (placeholder: a flash sphere is shown if unset). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<UNiagaraSystem> ExplosionEffect;

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
	TObjectPtr<UTextRenderComponent> FuseLabel;

private:
	void Explode();

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> BrokenActors;

	float FuseLeft = 0.f;
	float FlashLeft = 0.f;
	bool bArmed = false;
	bool bExploded = false;
};

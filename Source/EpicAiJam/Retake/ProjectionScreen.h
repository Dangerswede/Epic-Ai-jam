#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectionScreen.generated.h"

class UStaticMeshComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UMaterialInstanceDynamic;

/**
 * Set dressing with a story job (scene 13): a projection screen showing the room from the projector booth -
 * so the earlier takes (the ghosts) play on it, in a warm, flickering "dailies" look. No gameplay.
 * (Godot: a SubViewport with its own Camera3D, shown on a MeshInstance3D through a ViewportTexture.)
 */
UCLASS()
class EPICAIJAM_API AProjectionScreen : public AActor
{
	GENERATED_BODY()

public:
	AProjectionScreen();
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Screen size in cm (width, height). The screen faces the actor's +X. */
	UPROPERTY(EditAnywhere, Category = "Screen")
	FVector2D ScreenSize = FVector2D(600.f, 340.f);

	/** Where the projector "camera" sits and looks (world space). */
	UPROPERTY(EditAnywhere, Category = "Screen")
	FVector CaptureFrom = FVector(1000.f, -900.f, 450.f);

	UPROPERTY(EditAnywhere, Category = "Screen")
	FVector CaptureAt = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Screen")
	float Brightness = 6.f; // emissive on physical light units

	/** Live view of the room on the screen (the earlier takes replay in it). Off = a plain lit screen + caption. */
	UPROPERTY(EditAnywhere, Category = "Screen")
	bool bLiveCapture = true;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> ScreenMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Frame;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneCaptureComponent2D> Capture;
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UTextRenderComponent> Caption;
	UPROPERTY(Transient) TObjectPtr<UTextureRenderTarget2D> Target;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> ScreenMaterial;
	float Clock = 0.f;
};

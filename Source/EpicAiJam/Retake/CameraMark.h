#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CameraMark.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UMaterialInstanceDynamic;
struct FTakeData;

/**
 * F1: a director's camera mark. A tape ring on the floor plus a camera on a stand: be on the ring
 * while its time window is open (seconds into the take) and the shot is "in the can". Marks hit by
 * the kept takes raise the Director's rating, and the Director's Cut cuts between the mark cameras.
 * (Godot: an Area3D with a time window, plus a Marker3D for the camera position.)
 */
UCLASS()
class EPICAIJAM_API ACameraMark : public AActor
{
	GENERATED_BODY()

public:
	ACameraMark();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mark")
	int32 MarkNumber = 1;

	/** Short shot name shown in the Director's Cut ("Low angle on the bridge"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mark")
	FString ShotName;

	/** Window in seconds since the take started. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mark")
	float WindowStart = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mark")
	float WindowEnd = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mark")
	float Radius = 170.f;

	/** Camera position relative to the mark (also drawn as a camera on a stand). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mark", meta = (MakeEditWidget = "true"))
	FVector CameraLocation = FVector(-500.f, 300.f, 250.f);

	FVector GetCameraWorldLocation() const { return GetActorTransform().TransformPosition(CameraLocation); }

	/** First time (seconds into the take) the take stood on the mark inside the window, or < 0. */
	float FindHitTime(const FTakeData& Take) const;

	bool IsHitBy(const FTakeData& Take) const { return FindHitTime(Take) >= 0.f; }

	/** Is Location on the ring at take time T? */
	bool IsOnMark(const FVector& Location, float T) const;

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

private:
	void SetLook(const FLinearColor& Color, const FString& Label);

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Ring;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Dot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> CameraBody;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> CameraStand;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> RingMaterial;

	int32 LastTakeNumber = 0;
	bool bHitThisTake = false;
	float Pulse = 0.f;
};

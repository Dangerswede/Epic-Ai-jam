#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RetakeCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRetakeCharacterSignature);

/**
 * Player stunt double. Third-person camera on a spring arm, Enhanced Input for
 * move / look / jump / interact (E) / "Cut" (R).
 * Input actions are created in C++ at runtime unless overridden with assets in a Blueprint child.
 */
UCLASS()
class EPICAIJAM_API ARetakeCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ARetakeCharacter();

	/** Fired when the player presses Interact. */
	UPROPERTY(BlueprintAssignable, Category = "Retake")
	FRetakeCharacterSignature OnInteract;

	/** Fired when the player presses Cut (ends the current take). */
	UPROPERTY(BlueprintAssignable, Category = "Retake")
	FRetakeCharacterSignature OnCutRequested;

	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	UCameraComponent* GetFollowCamera() const { return FollowCamera; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** Placeholder body so the character is visible without art assets. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	// Optional asset overrides (set in a Blueprint child). If null, defaults are built in C++.
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> CutAction;

	UPROPERTY(EditAnywhere, Category = "Input", meta = (ClampMin = "0.0"))
	float MouseLookSensitivity = 1.0f;

private:
	void BuildDefaultInput();
	void AddMappingContext();

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Interact();
	void Cut();
};

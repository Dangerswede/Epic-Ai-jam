#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Retake/TakeData.h"
#include "RetakeCharacter.generated.h"

class URetakeRecorderComponent;
class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRetakeCharacterSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRetakeActionSignature, ERetakeAction, Action);

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

	/** Fired when the player presses Scrap (Backspace): throw away the last take. */
	UPROPERTY(BlueprintAssignable, Category = "Retake")
	FRetakeCharacterSignature OnScrapRequested;

	/** Fired whenever Space/Jump is pressed, even when a jump isn't possible (used for "roll" / "skip"). */
	UPROPERTY(BlueprintAssignable, Category = "Retake")
	FRetakeCharacterSignature OnJumpPressed;

	/** Fired for every recordable discrete action (Jump, Interact, Push). */
	UPROPERTY(BlueprintAssignable, Category = "Retake")
	FRetakeActionSignature OnActionPerformed;

	/** Report an action from outside (e.g. APushCrate reports Push). */
	UFUNCTION(BlueprintCallable, Category = "Retake")
	void NotifyAction(ERetakeAction Action) { OnActionPerformed.Broadcast(Action); }

	/** Jump / Interact / Cut only work while this is true (the director enables it only while Rolling). */
	UFUNCTION(BlueprintCallable, Category = "Retake")
	void SetActionsEnabled(bool bEnabled) { bActionsEnabled = bEnabled; }

	UFUNCTION(BlueprintPure, Category = "Retake")
	bool AreActionsEnabled() const { return bActionsEnabled; }

	URetakeRecorderComponent* GetRecorder() const { return Recorder; }
	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	UCameraComponent* GetFollowCamera() const { return FollowCamera; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Retake")
	TObjectPtr<URetakeRecorderComponent> Recorder;

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

	/** Backspace / B: scrap the last take. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ScrapAction;

	/** Esc / P / Start: pause menu (works while paused). */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> PauseAction;

	UPROPERTY(EditAnywhere, Category = "Input", meta = (ClampMin = "0.0"))
	float MouseLookSensitivity = 1.0f;

	/** Interact reach in cm (ghosts use the same value). */
	UPROPERTY(EditAnywhere, Category = "Retake")
	float InteractRadius = 175.f;

public:
	// Input handlers, public so automated playtests (ARetakeTestDriver) can press buttons.
	void StartJump();
	void Interact();
	void Cut();
	void Scrap();
	void TogglePause();

private:
	bool bActionsEnabled = true;

	void BuildDefaultInput();
	void AddMappingContext();

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
};

#include "Retake/RetakeCharacter.h"

#include "Retake/RetakeHUD.h"
#include "Retake/RetakeInteractable.h"
#include "Retake/RetakeRecorderComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "UObject/ConstructorHelpers.h"

ARetakeCharacter::ARetakeCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // only ticks while shaking

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	// Camera drives yaw of the view, character turns towards movement.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 540.f, 0.f);
	Move->JumpZVelocity = 600.f;
	Move->AirControl = 0.35f;
	Move->MaxWalkSpeed = 500.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 60.f);
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	Recorder = CreateDefaultSubobject<URetakeRecorderComponent>(TEXT("Recorder"));

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(RootComponent);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(CylinderMesh.Object);
		// Engine cylinder is 100x100x100 centered; scale to roughly match the capsule.
		BodyMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.9f));
	}
}

void ARetakeCharacter::BeginPlay()
{
	Super::BeginPlay();
	AddMappingContext();
}

void ARetakeCharacter::BuildDefaultInput()
{
	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = Type;
		return Action;
	};

	if (!MoveAction)     { MoveAction = MakeAction(TEXT("IA_Move"), EInputActionValueType::Axis2D); }
	if (!LookAction)     { LookAction = MakeAction(TEXT("IA_Look"), EInputActionValueType::Axis2D); }
	if (!JumpAction)     { JumpAction = MakeAction(TEXT("IA_Jump"), EInputActionValueType::Boolean); }
	if (!InteractAction) { InteractAction = MakeAction(TEXT("IA_Interact"), EInputActionValueType::Boolean); }
	if (!CutAction)      { CutAction = MakeAction(TEXT("IA_Cut"), EInputActionValueType::Boolean); }
	if (!ScrapAction)    { ScrapAction = MakeAction(TEXT("IA_Scrap"), EInputActionValueType::Boolean); }
	if (!PauseAction)
	{
		PauseAction = MakeAction(TEXT("IA_Pause"), EInputActionValueType::Boolean);
		PauseAction->bTriggerWhenPaused = true;
	}

	if (MappingContext)
	{
		return;
	}

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Retake"));

	// Move: X = right, Y = forward.
	auto MapMove = [this](FKey Key, bool bSwizzle, bool bNegate)
	{
		FEnhancedActionKeyMapping& Mapping = MappingContext->MapKey(MoveAction, Key);
		if (bSwizzle)
		{
			UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(MappingContext);
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			Mapping.Modifiers.Add(Swizzle);
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(MappingContext));
		}
	};
	MapMove(EKeys::W, true, false);
	MapMove(EKeys::S, true, true);
	MapMove(EKeys::D, false, false);
	MapMove(EKeys::A, false, true);
	MappingContext->MapKey(MoveAction, EKeys::Gamepad_Left2D);

	// Look: mouse Y is inverted so moving the mouse up looks up.
	{
		FEnhancedActionKeyMapping& Mouse = MappingContext->MapKey(LookAction, EKeys::Mouse2D);
		UInputModifierNegate* NegY = NewObject<UInputModifierNegate>(MappingContext);
		NegY->bX = false;
		NegY->bY = true;
		NegY->bZ = false;
		Mouse.Modifiers.Add(NegY);
	}
	MappingContext->MapKey(LookAction, EKeys::Gamepad_Right2D);

	MappingContext->MapKey(JumpAction, EKeys::SpaceBar);
	MappingContext->MapKey(JumpAction, EKeys::Gamepad_FaceButton_Bottom);

	MappingContext->MapKey(InteractAction, EKeys::E);
	MappingContext->MapKey(InteractAction, EKeys::Gamepad_FaceButton_Left);

	MappingContext->MapKey(CutAction, EKeys::R);
	MappingContext->MapKey(CutAction, EKeys::Gamepad_FaceButton_Top);

	MappingContext->MapKey(ScrapAction, EKeys::BackSpace);
	MappingContext->MapKey(ScrapAction, EKeys::Gamepad_FaceButton_Right);

	// Esc stops PIE in the editor, so P works there too.
	MappingContext->MapKey(PauseAction, EKeys::Escape);
	MappingContext->MapKey(PauseAction, EKeys::P);
	MappingContext->MapKey(PauseAction, EKeys::Gamepad_Special_Right);
}

void ARetakeCharacter::AddMappingContext()
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !MappingContext)
	{
		return;
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		Subsystem->RemoveMappingContext(MappingContext);
		Subsystem->AddMappingContext(MappingContext, 0);
	}
}

void ARetakeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	BuildDefaultInput();
	AddMappingContext();

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		UE_LOG(LogTemp, Error, TEXT("ARetakeCharacter needs EnhancedInputComponent (check DefaultInput.ini)."));
		return;
	}

	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ARetakeCharacter::Move);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ARetakeCharacter::Look);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ARetakeCharacter::StartJump);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	Input->BindAction(InteractAction, ETriggerEvent::Started, this, &ARetakeCharacter::Interact);
	Input->BindAction(CutAction, ETriggerEvent::Started, this, &ARetakeCharacter::Cut);
	Input->BindAction(ScrapAction, ETriggerEvent::Started, this, &ARetakeCharacter::Scrap);
	Input->BindAction(PauseAction, ETriggerEvent::Started, this, &ARetakeCharacter::TogglePause);
}

void ARetakeCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (!Controller)
	{
		return;
	}
	const FRotator YawRot(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FRotationMatrix RotMatrix(YawRot);
	AddMovementInput(RotMatrix.GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(RotMatrix.GetUnitAxis(EAxis::Y), Axis.X);
}

void ARetakeCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>() * MouseLookSensitivity;
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void ARetakeCharacter::StartJump()
{
	// Read BEFORE broadcasting: the broadcast may roll the take and enable actions this very frame.
	const bool bWasEnabled = bActionsEnabled;
	OnJumpPressed.Broadcast();
	if (!bWasEnabled)
	{
		return; // Space only rolls / skips here, it doesn't jump (and nothing gets recorded)
	}

	// Only record a Jump action when a jump can actually start (not mid-air spam).
	const bool bCouldJump = CanJump();
	Jump();
	if (bCouldJump)
	{
		NotifyAction(ERetakeAction::Jump);
	}
}

void ARetakeCharacter::Interact()
{
	if (!bActionsEnabled)
	{
		return;
	}
	RetakeInteraction::PerformInteract(this, InteractRadius);
	OnInteract.Broadcast();
	NotifyAction(ERetakeAction::Interact);
}

void ARetakeCharacter::PlayStuntShake(float Duration, float Amplitude)
{
	if (ShakeTimeLeft <= 0.f)
	{
		BaseSocketOffset = CameraBoom->SocketOffset;
	}
	ShakeDuration = FMath::Max(0.01f, Duration);
	ShakeTimeLeft = ShakeDuration;
	ShakeAmplitude = Amplitude;
	SetActorTickEnabled(true);
}

void ARetakeCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (ShakeTimeLeft <= 0.f)
	{
		SetActorTickEnabled(false);
		return;
	}
	// Real time, so the shake isn't stretched by the slow-mo.
	const float Dilation = FMath::Max(0.01f, GetActorTimeDilation());
	ShakeTimeLeft -= DeltaSeconds / Dilation;
	if (ShakeTimeLeft <= 0.f)
	{
		CameraBoom->SocketOffset = BaseSocketOffset;
		return;
	}
	const float Strength = ShakeAmplitude * (ShakeTimeLeft / ShakeDuration);
	CameraBoom->SocketOffset = BaseSocketOffset + FVector(0.f, FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)) * Strength;
}

void ARetakeCharacter::Scrap()
{
	if (!bActionsEnabled)
	{
		return;
	}
	OnScrapRequested.Broadcast();
}

void ARetakeCharacter::TogglePause()
{
	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ARetakeHUD* HUD = PC->GetHUD<ARetakeHUD>())
		{
			HUD->TogglePauseMenu();
		}
	}
}

void ARetakeCharacter::Cut()
{
	if (!bActionsEnabled)
	{
		return;
	}
	OnCutRequested.Broadcast();
}

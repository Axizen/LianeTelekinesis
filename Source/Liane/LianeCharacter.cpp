// Copyright Epic Games, Inc. All Rights Reserved.

#include "LianeCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "TelekinesisComponent.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ALianeCharacter

ALianeCharacter::ALianeCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Create the telekinesis component
	TelekinesisComponent = CreateDefaultSubobject<UTelekinesisComponent>(TEXT("TelekinesisComponent"));

	// Initialize state variables
	bIsAiming = false;
	HeldObjectDistance = 300.0f;

	// Set this character to call Tick() every frame
	PrimaryActorTick.bCanEverTick = true;
}

void ALianeCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// Initialize telekinesis state
	bIsAiming = false;
	HeldObjectDistance = 300.0f;
}

void ALianeCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update telekinesis target location if we're holding an object
	if (TelekinesisComponent && TelekinesisComponent->IsObjectGrabbed())
	{
		// Update the target location based on where the player is looking
		FVector CameraLocation = FollowCamera->GetComponentLocation();
		FVector CameraForward = FollowCamera->GetForwardVector();
		FVector TargetLocation = CameraLocation + (CameraForward * HeldObjectDistance); // Use variable distance

		TelekinesisComponent->MoveObjectToLocation(TargetLocation);
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void ALianeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALianeCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ALianeCharacter::Look);

		// Telekinesis Grab/Throw
		if (TelekinesisGrabAction)
		{
			EnhancedInputComponent->BindAction(TelekinesisGrabAction, ETriggerEvent::Started, this, &ALianeCharacter::TelekinesisGrab);
		}

		// Telekinesis Aim
		if (TelekinesisAimAction)
		{
			EnhancedInputComponent->BindAction(TelekinesisAimAction, ETriggerEvent::Started, this, &ALianeCharacter::TelekinesisAimStart);
			EnhancedInputComponent->BindAction(TelekinesisAimAction, ETriggerEvent::Completed, this, &ALianeCharacter::TelekinesisAimEnd);
		}

		// Telekinesis Release
		if (TelekinesisReleaseAction)
		{
			EnhancedInputComponent->BindAction(TelekinesisReleaseAction, ETriggerEvent::Started, this, &ALianeCharacter::TelekinesisRelease);
		}

		// Reset Object Position
		if (ResetObjectPositionAction)
		{
			EnhancedInputComponent->BindAction(ResetObjectPositionAction, ETriggerEvent::Started, this, &ALianeCharacter::ResetObjectPosition);
		}

		// Adjust Object Distance
		if (AdjustObjectDistanceAction)
		{
			EnhancedInputComponent->BindAction(AdjustObjectDistanceAction, ETriggerEvent::Triggered, this, &ALianeCharacter::AdjustObjectDistance);
		}
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ALianeCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ALianeCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ALianeCharacter::TelekinesisGrab(const FInputActionValue& Value)
{
	if (!TelekinesisComponent)
		return;

	// If already holding an object and aiming, throw it
	if (TelekinesisComponent->IsObjectGrabbed() && bIsAiming)
	{
		TelekinesisComponent->ThrowObject();
	}
	// Otherwise try to grab an object
	else
	{
		TelekinesisComponent->GrabObject();
	}
}

void ALianeCharacter::TelekinesisAimStart(const FInputActionValue& Value)
{
	bIsAiming = true;
	
	// If holding an object, start charging throw
	if (TelekinesisComponent && TelekinesisComponent->IsObjectGrabbed())
	{
		TelekinesisComponent->StartChargeThrow();
	}
}

void ALianeCharacter::TelekinesisAimEnd(const FInputActionValue& Value)
{
	bIsAiming = false;
}

void ALianeCharacter::TelekinesisRelease(const FInputActionValue& Value)
{
	// Release the object without throwing
	if (TelekinesisComponent && TelekinesisComponent->IsObjectGrabbed())
	{
		TelekinesisComponent->ReleaseObject(false);
	}
}

void ALianeCharacter::ResetObjectPosition(const FInputActionValue& Value)
{
	// Reset the held object to default position
	if (TelekinesisComponent && TelekinesisComponent->IsObjectGrabbed())
	{
		HeldObjectDistance = 300.0f; // Reset to default distance
	}
}

void ALianeCharacter::AdjustObjectDistance(const FInputActionValue& Value)
{
	// Get the input value (1.0 for wheel up, -1.0 for wheel down)
	float AdjustValue = Value.Get<float>();
	
	// Only process if we're holding an object
	if (TelekinesisComponent && TelekinesisComponent->IsObjectGrabbed())
	{
		// Adjust distance based on wheel direction
		// Multiply by 50 for a reasonable adjustment amount per tick
		HeldObjectDistance = FMath::Clamp(HeldObjectDistance + (AdjustValue * 50.0f), 100.0f, 1000.0f);
	}
}
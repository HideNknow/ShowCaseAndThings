// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShowCaseAndThingsCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"

//////////////////////////////////////////////////////////////////////////
// AShowCaseAndThingsCharacter

AShowCaseAndThingsCharacter::AShowCaseAndThingsCharacter(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer.SetDefaultSubobjectClass<UCustomMovementComponent>(ACharacter::CharacterMovementComponentName))

{
	CustomMovementComponent = Cast<UCustomMovementComponent>(GetCharacterMovement());


	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	TurnRateGamepad = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));
	

}

#pragma region Input

void AShowCaseAndThingsCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("SetupPlayerInputComponent")));
	// Set up gameplay key bindings
	check(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		//Jumping
		EnhancedInputComponent->BindAction(JumpAction,		ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction,		ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//Moving
		EnhancedInputComponent->BindAction(MoveAction,		ETriggerEvent::Triggered, this, &AShowCaseAndThingsCharacter::Move);

		//Looking
		EnhancedInputComponent->BindAction(LookAction,		ETriggerEvent::Triggered, this, &AShowCaseAndThingsCharacter::Look);
		
		//Interacting
		EnhancedInputComponent->BindAction(InteractAction,	ETriggerEvent::Triggered, this, &AShowCaseAndThingsCharacter::Interact);

		//Crouching
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Triggered, this->CustomMovementComponent, &UCustomMovementComponent::CrouchPressed);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this->CustomMovementComponent, &UCustomMovementComponent::CrouchReleased);

		//Sprinting
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Triggered, this->CustomMovementComponent, &UCustomMovementComponent::SprintPressed);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this->CustomMovementComponent, &UCustomMovementComponent::SprintReleased);
	}

	// Enable touchscreen input
	//EnableTouchscreenMovement(PlayerInputComponent);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "Mouse" versions handle devices that provide an absolute delta, such as a mouse.
	// "Gamepad" versions are for devices that we choose to treat as a rate of change, such as an analog joystick
	// PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this, &AShowCaseAndThingsCharacter::TurnAtRate);
	// PlayerInputComponent->BindAxis("Look Up / Down Gamepad", this, &AShowCaseAndThingsCharacter::LookUpAtRate);
}

void AShowCaseAndThingsCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void AShowCaseAndThingsCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y * -1.0f);
	}
}

void AShowCaseAndThingsCharacter::Interact(const FInputActionValue& Value)
{
	OnUseItem.Broadcast();
}

void AShowCaseAndThingsCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == true)
	{
		return;
	}
	if ((FingerIndex == TouchItem.FingerIndex) && (TouchItem.bMoved == false))
	{
		OnUseItem.Broadcast();
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void AShowCaseAndThingsCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	TouchItem.bIsPressed = false;
}

// void AShowCaseAndThingsCharacter::TurnAtRate(float Rate)
// {
// 	// calculate delta for this frame from the rate information
// 	AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
// }

// void AShowCaseAndThingsCharacter::LookUpAtRate(float Rate)
// {
// 	// calculate delta for this frame from the rate information
// 	AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
// }

bool AShowCaseAndThingsCharacter::EnableTouchscreenMovement(class UInputComponent* PlayerInputComponent)
{
	if (FPlatformMisc::SupportsTouchInput() || GetDefault<UInputSettings>()->bUseMouseForTouch)
	{
		PlayerInputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AShowCaseAndThingsCharacter::BeginTouch);
		PlayerInputComponent->BindTouch(EInputEvent::IE_Released, this, &AShowCaseAndThingsCharacter::EndTouch);

		return true;
	}
	
	return false;
}

#pragma endregion

void AShowCaseAndThingsCharacter::BeginPlay()
{
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	Super::BeginPlay();
	
}


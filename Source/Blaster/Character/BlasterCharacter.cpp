// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/Input/BlasterInputConfigData.h"
#include "Blaster/BlasterComponent/CombatComponent.h"
#include "Kismet/KismetMathLibrary.h"

#define print(text) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.5, FColor::White,text)

// Sets default values
ABlasterCharacter::ABlasterCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	CombatComponent->SetIsReplicated(true);

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (CombatComponent)
	{
		CombatComponent->Character = this;
	}
}

// Called when the game starts or when spawned
void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AimOffset(DeltaTime);
}

// Called to bind functionality to input
void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	// Get the player controller
	APlayerController* PC = Cast<APlayerController>(GetController());

	// Get the local player subsystem
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());
	// Clear out existing mapping, and add our mapping
	Subsystem->ClearAllMappings();
	Subsystem->AddMappingContext(DefaultMappingContext, 0);

	UEnhancedInputComponent* PEI = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	// Bind the actions
	PEI->BindAction(InputConfig->InputMove, ETriggerEvent::Triggered, this, &ABlasterCharacter::Move);
	PEI->BindAction(InputConfig->InputLook, ETriggerEvent::Triggered, this, &ABlasterCharacter::Look);
	PEI->BindAction(InputConfig->Jump, ETriggerEvent::Triggered, this, &ABlasterCharacter::JumpAction);
	PEI->BindAction(InputConfig->Interact, ETriggerEvent::Triggered, this, &ABlasterCharacter::Interact);
	PEI->BindAction(InputConfig->Crouch, ETriggerEvent::Triggered, this, &ABlasterCharacter::CrouchAction);
	PEI->BindAction(InputConfig->Aim, ETriggerEvent::Started, this, &ABlasterCharacter::AimButtonPressed);
	PEI->BindAction(InputConfig->Aim, ETriggerEvent::Completed, this, &ABlasterCharacter::AimButtonReleased);

	/*PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &ABlasterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &ABlasterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &ABlasterCharacter::Turn);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &ABlasterCharacter::LookUp);*/
}

void ABlasterCharacter::Move(const FInputActionValue& Value)
{
	if (Controller != nullptr)
	{
		const FVector2D MoveValue = Value.Get<FVector2D>();
		const FRotator MovementRotation(0, Controller->GetControlRotation().Yaw, 0);

		// Forward/Backward direction
		if (MoveValue.Y != 0.f)
		{
			// Get forward vector
			const FVector Direction = MovementRotation.RotateVector(FVector::ForwardVector);

			AddMovementInput(Direction, MoveValue.Y);
		}

		// Right/Left direction
		if (MoveValue.X != 0.f)
		{
			// Get right vector
			const FVector Direction = MovementRotation.RotateVector(FVector::RightVector);

			AddMovementInput(Direction, MoveValue.X);
		}
	}
}

void ABlasterCharacter::Look(const FInputActionValue& Value)
{
	if (Controller != nullptr)
	{
		const FVector2D LookValue = Value.Get<FVector2D>();

		if (LookValue.X != 0.f)
		{
			AddControllerYawInput(LookValue.X);
		}

		if (LookValue.Y != 0.f)
		{
			AddControllerPitchInput(LookValue.Y);
		}
	}
}

void ABlasterCharacter::JumpAction(const FInputActionValue& Value)
{
	Jump();
}

void ABlasterCharacter::Interact(const FInputActionValue& Value)
{
	print(FString::Printf(TEXT("Interact")));

	if (!CombatComponent)
	{
		print(FString::Printf(TEXT("Don't have CombatComponent")));
		return;
	}

	print(FString::Printf(TEXT("CombatComponent is present")));

	if (!HasAuthority())
	{
		print(FString::Printf(TEXT("Don't have authority")));
		ServerInteract();
	}
	else
	{
		CombatComponent->EquipWeapon(OverlappingWeapon);
	}
}

void ABlasterCharacter::CrouchAction(const FInputActionValue& Value)
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ABlasterCharacter::AimButtonReleased(const FInputActionValue& Value)
{
	if (CombatComponent)
	{
		CombatComponent->SetAiming(false);
	}
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if (CombatComponent && CombatComponent->EquippedWeapon == nullptr) return;
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;

	float Speed = Velocity.Size();

	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !bIsInAir) // standing still
	{
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		bUseControllerRotationYaw = true;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		TurnInPlace(DeltaTime);
	}

	if (Speed > 0.f || bIsInAir) // running, or jumping
	{
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	AO_Pitch = GetBaseAimRotation().Pitch;

	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map pitch from [270, 360) to [-90, 0)
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABlasterCharacter::AimButtonPressed(const FInputActionValue& Value)
{
	if (CombatComponent)
	{
		CombatComponent->SetAiming(true);
	}
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

void ABlasterCharacter::ServerInteract_Implementation()
{
	CombatComponent->EquipWeapon(OverlappingWeapon);
}

void ABlasterCharacter::SetOverllapingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}

	OverlappingWeapon = Weapon;

	if (IsLocallyControlled() && OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
}

bool ABlasterCharacter::IsWeaponEquipped()
{
	return (CombatComponent && CombatComponent->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming()
{
	return CombatComponent->bIsAiming;
}

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
	if (CombatComponent == nullptr) return nullptr;
	return CombatComponent->EquippedWeapon;
}



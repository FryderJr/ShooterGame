// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;
}


// Called when the game starts
void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
	
}

void UCombatComponent::SetAiming(bool bNewIsAiming)
{
	bIsAiming = bNewIsAiming;
	ServerSetAiming(bNewIsAiming);
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bNewIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bNewIsAiming)
{
	bIsAiming = bNewIsAiming;
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bNewIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}


// Called every frame
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bIsAiming);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr)
	{
		return;
	}
	
	const USkeletalMeshSocket* HandSocket =  Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		EquippedWeapon = WeaponToEquip;
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
		EquippedWeapon->SetOwner(Character);
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}


// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BlasterInputConfigData.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UBlasterInputConfigData : public UDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UInputAction* InputMove;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UInputAction* InputLook;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UInputAction* Jump;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UInputAction* Interact;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UInputAction* Crouch;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UInputAction* Aim;
};

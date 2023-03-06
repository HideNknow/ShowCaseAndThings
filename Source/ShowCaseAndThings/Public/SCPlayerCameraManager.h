// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "./CustomMovementComponent.h"
#include "../ShowCaseAndThingsCharacter.h"

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "SCPlayerCameraManager.generated.h"

/**
 * 
 */
UCLASS()
class SHOWCASEANDTHINGS_API ASCPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly) float CrouchBlendDuration = .5f;
	float CrouchBlendTime;
	

public:
	ASCPlayerCameraManager();

	virtual void UpdateViewTarget(FTViewTarget)


};

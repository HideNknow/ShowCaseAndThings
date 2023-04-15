// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class SHOWCASEANDTHINGS_API CustomCharacter
{
public:
	CustomCharacter();
	~CustomCharacter();

	//Unreal Classes
	class USpringArmComponent;
	class UCameraComponent;

	//Custom Classes
	class AShowCaseAndThingsCharacter;
	class UCustomMovementComponent;
	class ASCPlayerCameraManager;
};

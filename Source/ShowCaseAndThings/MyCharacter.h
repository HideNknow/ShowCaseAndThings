// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MyMovementComponent.h"
#include "MyCharacter.generated.h"

UCLASS()
class SHOWCASEANDTHINGS_API AMyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMyCharacter(const FObjectIntializer& ObjectIntializer);

	UFUNCTION(BlueprintCallable, Category = "Movement")
		FORCEINLINE class UMyMovementComponent* GetMyMovementComponent() const { return MyMovementComponent; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	UMyMovementComponent* MyMovementComponent;

};

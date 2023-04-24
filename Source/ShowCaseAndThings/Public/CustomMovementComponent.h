// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Matthieu/CustomCharacter.h"


#include "GameFramework/CharacterMovementComponent.h"
#include "CustomMovementComponent.generated.h"


UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_None			UMETA(Hidden),
	CMOVE_Slide			UMETA(DisplayName = "Slide"),
	CMOVE_Prone			UMETA(DisplayName = "Prone"),
	CMOVE_WallRun		UMETA(DisplayName = "Wall Run"),
	CMOVE_Hang			UMETA(DisplayName = "Hang"),
	CMOVE_Climb			UMETA(DisplayName = "Climb"),
	CMOVE_MAX			UMETA(Hidden),
};

/**
 * 
 */
UCLASS()
class SHOWCASEANDTHINGS_API UCustomMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
		
		class FSavedMove_Zippy : public FSavedMove_Character
	{
	public:
		enum CompressedFlags
		{
			FLAG_Sprint = 0x10,
			FLAG_Dash = 0x20,
			FLAG_Custom_2 = 0x40,
			FLAG_Custom_3 = 0x80,
		};

		// Flags
		uint8 Saved_bPressedZippyJump : 1;
		uint8 Saved_bWantsToSprint : 1;
		uint8 Saved_bWantsToDash : 1;

		// Other Variables
		uint8 Saved_bHadAnimRootMotion : 1;
		uint8 Saved_bTransitionFinished : 1;
		uint8 Saved_bWantsToProne : 1;
		uint8 Saved_bWallRunIsRight : 1;


		FSavedMove_Zippy();

		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
		virtual void Clear() override;
		virtual uint8 GetCompressedFlags() const override;
		virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
		virtual void PrepMoveFor(ACharacter* C) override;
	};

	class FNetworkPredictionData_Client_Zippy : public FNetworkPredictionData_Client_Character
	{
	public:
		FNetworkPredictionData_Client_Zippy(const UCharacterMovementComponent  & ClientMovement);

		typedef FNetworkPredictionData_Client_Character Super;

		virtual FSavedMovePtr AllocateNewMove() override;
	};

	UPROPERTY(EditDefaultsOnly) float Sprint_MaxWalkSpeed;
	UPROPERTY(EditDefaultsOnly) float Walk_MaxWalkSpeed;

	bool Safe_bWantsToSprint;

public:
	UCustomMovementComponent();

protected:
	virtual void InitializeComponent() override;

public:
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	virtual bool IsMovingOnGround() const override;
	virtual bool CanCrouchInCurrentState() const override;

protected:
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

//for Movement
public:
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;

protected:
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

#pragma region Slide
private:
	void EnterSlide(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode);
	void ExitSlide();
	//Le joueur peut il slide en f(Vitesse & Sol)
	bool CanSlide() const;
	void PhysSlide(float deltaTime, int32 Iterations);


	// Slide
	UPROPERTY(EditDefaultsOnly, Category = "Slide") float MinSlideSpeed = 400.f;
	UPROPERTY(EditDefaultsOnly, Category = "Slide") float SlideEnterImpulse = 400.f;
	UPROPERTY(EditDefaultsOnly, Category = "Slide") float SlideGravityForce = 4000.f;
	//Multiplie GroundFriction*SlideFrictionFactor
	UPROPERTY(EditDefaultsOnly, Category = "Slide") float SlideFrictionFactor = .06f;
	UPROPERTY(EditDefaultsOnly ,Category = "Slide")float SlideControl = 1.0f;


FCollisionQueryParams GetIgnoreCharacterParams() const;

#pragma endregion 


public:
	UFUNCTION(BlueprintCallable) void SprintPressed();
	UFUNCTION(BlueprintCallable) void SprintReleased();
	UFUNCTION(BlueprintCallable) void CrouchPressed();
	UFUNCTION(BlueprintCallable) void CrouchReleased();
	UFUNCTION(BlueprintCallable) void CustomDisplayDebug();

	UFUNCTION(BlueprintPure) bool IsCustomMovementMode(ECustomMovementMode InCustomMovementMode) const;
	UFUNCTION(BlueprintPure) bool IsMovementMode(EMovementMode InMovementMode) const;



};

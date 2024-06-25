// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomMovementComponent.h"

#include "Macros.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"


void UCustomMovementComponent::InitializeComponent()
{
}


#pragma region Saved Move

UCustomMovementComponent::FSavedMove_Zippy::FSavedMove_Zippy()
{
	Saved_bWantsToSprint = 0;
}

bool UCustomMovementComponent::FSavedMove_Zippy::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	const FSavedMove_Zippy* NewZippyMove = static_cast<FSavedMove_Zippy*>(NewMove.Get());

	if (Saved_bWantsToSprint != NewZippyMove->Saved_bWantsToSprint)
	{
		return false;
	}

	if (Saved_bWantsToDash != NewZippyMove->Saved_bWantsToDash)
	{
		return false;
	}

	if (Saved_bWallRunIsRight != NewZippyMove->Saved_bWallRunIsRight)
	{
		return false;
	}

	return FSavedMove_Character::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void UCustomMovementComponent::FSavedMove_Zippy::Clear()
{
	FSavedMove_Character::Clear();

	Saved_bWantsToSprint = 0;
	Saved_bWantsToDash = 0;
	Saved_bPressedZippyJump = 0;

	Saved_bHadAnimRootMotion = 0;
	Saved_bTransitionFinished = 0;

	Saved_bWantsToProne = 0;

	Saved_bWallRunIsRight = 0;
}

uint8 UCustomMovementComponent::FSavedMove_Zippy::GetCompressedFlags() const
{
	uint8 Result = FSavedMove_Character::GetCompressedFlags();

	if (Saved_bWantsToSprint) Result |= FLAG_Sprint;
	if (Saved_bWantsToDash) Result |= FLAG_Dash;
	if (Saved_bPressedZippyJump) Result |= FLAG_JumpPressed;

	return Result;
}

void UCustomMovementComponent::FSavedMove_Zippy::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	FSavedMove_Character::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	const UCustomMovementComponent* CharacterMovement = Cast<UCustomMovementComponent>(C->GetCharacterMovement());

	Saved_bWantsToSprint = CharacterMovement->Safe_bWantsToSprint;
	
}

void UCustomMovementComponent::FSavedMove_Zippy::PrepMoveFor(ACharacter* C)
{

	FSavedMove_Character::PrepMoveFor(C);

	UCustomMovementComponent* CharacterMovement = Cast<UCustomMovementComponent>(C->GetCharacterMovement());

	CharacterMovement->Safe_bWantsToSprint = Saved_bWantsToSprint;
	
}

#pragma endregion

#pragma region Client Network Prediction Data

UCustomMovementComponent::UCustomMovementComponent()
{
	NavAgentProps.bCanCrouch = true;
}

UCustomMovementComponent::FNetworkPredictionData_Client_Zippy::FNetworkPredictionData_Client_Zippy(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
}

FSavedMovePtr UCustomMovementComponent::FNetworkPredictionData_Client_Zippy::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_Zippy());
}

FNetworkPredictionData_Client* UCustomMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != nullptr)

		if (ClientPredictionData == nullptr)
		{
			UCustomMovementComponent* MutableThis = const_cast<UCustomMovementComponent*>(this);

			MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Zippy(*this);
			MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
			MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;
		}
	return ClientPredictionData;
}

bool UCustomMovementComponent::IsMovingOnGround() const
{
	return Super::IsMovingOnGround() || IsCustomMovementMode(CMOVE_Slide);
}

bool UCustomMovementComponent::CanCrouchInCurrentState() const
{
	//ajouter les conditions a la suite du Supper comme dans un if()
	return Super::CanCrouchInCurrentState();
}

void UCustomMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	Safe_bWantsToSprint = (Flags & FSavedMove_Zippy::FLAG_Sprint) != 0;
}


#pragma endregion

//cette fonction est appeler tout les ticks
void UCustomMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	// Slide Condition
	if (MovementMode == MOVE_Walking && bWantsToCrouch)
	{
		if (CanSlide())
		{
			SetMovementMode(MOVE_Custom, CMOVE_Slide);
		}
	}
	//Sortir du slide
	else if (IsCustomMovementMode(CMOVE_Slide) && !bWantsToCrouch)
	{
		
		SetMovementMode(MOVE_Walking);
	}

	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

void UCustomMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);

	if (MovementMode == MOVE_Walking)
	{
		if (Safe_bWantsToSprint)
		{
			MaxWalkSpeed = Sprint_MaxWalkSpeed;
		}
		else
		{
			MaxWalkSpeed = Walk_MaxWalkSpeed;
		}
	}


	this->CustomDisplayDebug();

}

void UCustomMovementComponent::CustomDisplayDebug()
{
	// FString strg_Movement = GetMovementName();
	// if (GetMovementName() == FString("Custom"))
	// {
	// 	UEnum* MyEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("ECustomMovementMode"), true);
	// 	strg_Movement = MyEnum->GetNameStringByValue(CustomMovementMode);
	// }
	// GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, strg_Movement);
	//
	// GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, (TEXT("Vel %s"), FString::SanitizeFloat(Velocity.Size())));
	// GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, (TEXT("CanCrouch %s"),CanCrouchInCurrentState() ? TEXT("true") : TEXT("false")));
	//
	// DrawDebugLine(GetWorld(), GetOwner()->GetActorLocation(), GetOwner()->GetActorLocation() + (Acceleration.GetSafeNormal2D() * 100), FColor::Blue, false, 0.f, -1.f, 1.f);

}

void UCustomMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);

	switch (CustomMovementMode)
	{
	case CMOVE_Slide:
		PhysSlide(deltaTime, Iterations);
		break;
	default:
		UE_LOG(LogTemp, Fatal, TEXT("Invalid Movement Mode"))
	}
}


#pragma region input
void UCustomMovementComponent::SprintPressed()
{
	Safe_bWantsToSprint = true;
}

void UCustomMovementComponent::SprintReleased()
{
	Safe_bWantsToSprint = false;
}

void UCustomMovementComponent::CrouchPressed()
{
	bWantsToCrouch = true;
}

void UCustomMovementComponent::CrouchReleased()
{
	bWantsToCrouch = false;
}


bool UCustomMovementComponent::IsCustomMovementMode(ECustomMovementMode InCustomMovementMode) const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == InCustomMovementMode;
}

bool UCustomMovementComponent::IsMovementMode(EMovementMode InMovementMode) const
{
	return false;
}

#pragma endregion 

#pragma region Slide
bool UCustomMovementComponent::CanSlide() const
{
	FVector Start = UpdatedComponent->GetComponentLocation();
	FVector End = Start + CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.5f * FVector::DownVector;
	FName ProfileName = TEXT("BlockAll");
	bool bValidSurface = GetWorld()->LineTraceTestByProfile(Start, End, ProfileName, this->GetIgnoreCharacterParams());
	bool bEnoughSpeed = LastUpdateVelocity.Size() > MinSlideSpeed;

	//j'ai du retirer bValidSurface car il nous empeche de sauter d'un ledge quand on slide
	//return bValidSurface && bEnoughSpeed;
	return bEnoughSpeed;
}

void UCustomMovementComponent::EnterSlide(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode)
{
	bWantsToCrouch = true;
	bOrientRotationToMovement = false;
	Velocity += Velocity.GetSafeNormal2D() * SlideEnterImpulse;

	FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, true, NULL);
}

void UCustomMovementComponent::ExitSlide()
{
	
	bWantsToCrouch = false;
	bOrientRotationToMovement = true;
}


void UCustomMovementComponent::PhysSlide(float deltaTime, int32 Iterations)
{
	//On ne fait pas de calcul de phys si le delta < au minTickTime
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	//Si on ne peut pas Slide
	if (!CanSlide())
	{
		SetMovementMode(MOVE_Walking);
		StartNewPhysics(deltaTime, Iterations);
		
		return;
	}

	bJustTeleported = false;
	bool bCheckedFall = false;
	bool bTriedLedgeMove = false;
	float remainingTime = deltaTime;

	// Perform the move
	while ((remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) && CharacterOwner && (CharacterOwner->Controller || bRunPhysicsWithNoController || (CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)))
	{
		Iterations++;
		bJustTeleported = false;
		const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;

		// Save current values
		UPrimitiveComponent* const OldBase = GetMovementBase();
		const FVector PreviousBaseLocation = (OldBase != NULL) ? OldBase->GetComponentLocation() : FVector::ZeroVector;
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FFindFloorResult OldFloor = CurrentFloor;

		// Ensure velocity is horizontal.
		MaintainHorizontalGroundVelocity();
		const FVector OldVelocity = Velocity;

		FVector SlopeForce = CurrentFloor.HitResult.Normal;
		SlopeForce.Z = 0.f;
		Velocity += SlopeForce * SlideGravityForce * deltaTime;
		Acceleration = (Acceleration * SlideControl).ProjectOnTo(UpdatedComponent->GetRightVector().GetSafeNormal());

		// Apply acceleration
		CalcVelocity(timeTick, GroundFriction * SlideFrictionFactor, false, GetMaxBrakingDeceleration());

		// Compute move parameters
		const FVector MoveVelocity = Velocity;
		const FVector Delta = timeTick * MoveVelocity;
		const bool bZeroDelta = Delta.IsNearlyZero();
		FStepDownResult StepDownResult;
		bool bFloorWalkable = CurrentFloor.IsWalkableFloor();

		if (bZeroDelta)
		{
			remainingTime = 0.f;
		}
		else
		{
			// try to move forward
			MoveAlongFloor(MoveVelocity, timeTick, &StepDownResult);

			if (IsFalling())
			{
				// pawn decided to jump up
				const float DesiredDist = Delta.Size();

				if (DesiredDist > KINDA_SMALL_NUMBER)
				{
					const float ActualDist = (UpdatedComponent->GetComponentLocation() - OldLocation).Size2D();
					remainingTime += timeTick * (1.f - FMath::Min(1.f, ActualDist / DesiredDist));
				}

				StartNewPhysics(remainingTime, Iterations);
				return;
			}
			else if (IsSwimming()) //just entered water
			{
				StartSwimming(OldLocation, OldVelocity, timeTick, remainingTime, Iterations);
				return;
			}
		}

		// Update floor.
		// StepUp might have already done it for us.
		if (StepDownResult.bComputedFloor)
		{
			CurrentFloor = StepDownResult.FloorResult;
		}
		else
		{
			FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, bZeroDelta, NULL);
		}


		// check for ledges here
		const bool bCheckLedges = !CanWalkOffLedges();
		if (bCheckLedges && !CurrentFloor.IsWalkableFloor())
		{
			// calculate possible alternate movement
			const FVector GravDir = FVector(0.f, 0.f, -1.f);
			const FVector NewDelta = bTriedLedgeMove ? FVector::ZeroVector : GetLedgeMove(OldLocation, Delta, GravDir);
			if (!NewDelta.IsZero())
			{
				// first revert this move
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, false);

				// avoid repeated ledge moves if the first one fails
				bTriedLedgeMove = true;

				// Try new movement direction
				Velocity = NewDelta / timeTick;
				remainingTime += timeTick;
				continue;
			}
			else
			{
				// see if it is OK to jump
				// @todo collision : only thing that can be problem is that oldbase has world collision on
				bool bMustJump = bZeroDelta || (OldBase == NULL || (!OldBase->IsQueryCollisionEnabled() && MovementBaseUtility::IsDynamicBase(OldBase)));
				if ((bMustJump || !bCheckedFall) && CheckFall(OldFloor, CurrentFloor.HitResult, Delta, OldLocation, remainingTime, timeTick, Iterations, bMustJump))
				{
					return;
				}
				bCheckedFall = true;

				// revert this move
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, true);
				remainingTime = 0.f;
				break;
			}
		}
		else
		{
			// Validate the floor check
			if (CurrentFloor.IsWalkableFloor())
			{
				if (ShouldCatchAir(OldFloor, CurrentFloor))
				{
					HandleWalkingOffLedge(OldFloor.HitResult.ImpactNormal, OldFloor.HitResult.Normal, OldLocation, timeTick);
					if (IsMovingOnGround())
					{
						// If still walking, then fall. If not, assume the user set a different mode they want to keep.
						StartFalling(Iterations, remainingTime, timeTick, Delta, OldLocation);
					}
					return;
				}

				AdjustFloorHeight();
				SetBase(CurrentFloor.HitResult.Component.Get(), CurrentFloor.HitResult.BoneName);
			}
			else if (CurrentFloor.HitResult.bStartPenetrating && remainingTime <= 0.f)
			{
				// The floor check failed because it started in penetration
				// We do not want to try to move downward because the downward sweep failed, rather we'd like to try to pop out of the floor.
				FHitResult Hit(CurrentFloor.HitResult);
				Hit.TraceEnd = Hit.TraceStart + FVector(0.f, 0.f, MAX_FLOOR_DIST);
				const FVector RequestedAdjustment = GetPenetrationAdjustment(Hit);
				ResolvePenetration(RequestedAdjustment, Hit, UpdatedComponent->GetComponentQuat());
				bForceNextFloorCheck = true;
			}

			// check if just entered water
			if (IsSwimming())
			{
				StartSwimming(OldLocation, Velocity, timeTick, remainingTime, Iterations);
				return;
			}

			// See if we need to start falling.
			if (!CurrentFloor.IsWalkableFloor() && !CurrentFloor.HitResult.bStartPenetrating)
			{
				const bool bMustJump = bJustTeleported || bZeroDelta || (OldBase == NULL || (!OldBase->IsQueryCollisionEnabled() && MovementBaseUtility::IsDynamicBase(OldBase)));
				if ((bMustJump || !bCheckedFall) && CheckFall(OldFloor, CurrentFloor.HitResult, Delta, OldLocation, remainingTime, timeTick, Iterations, bMustJump))
				{
					return;
				}
				bCheckedFall = true;
			}
		}

		// Allow overlap events and such to change physics state and velocity
		if (IsMovingOnGround() && bFloorWalkable)
		{
			// Make velocity reflect actual move
			if (!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && timeTick >= MIN_TICK_TIME)
			{
				// TODO-RootMotionSource: Allow this to happen during partial override Velocity, but only set allowed axes?
				Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / timeTick;
				MaintainHorizontalGroundVelocity();
			}
		}

		// If we didn't move at all this iteration then abort (since future iterations will also be stuck).
		if (UpdatedComponent->GetComponentLocation() == OldLocation)
		{
			remainingTime = 0.f;
			break;
		}
	}


	FHitResult Hit;
	//FQuat NewRotation = FRotationMatrix::MakeFromXZ(Velocity.GetSafeNormal2D(), FVector::UpVector).ToQuat();
	FQuat NewRotation = UpdatedComponent->GetComponentQuat();
	SafeMoveUpdatedComponent(FVector::ZeroVector, NewRotation, false, Hit);
}

FCollisionQueryParams UCustomMovementComponent::GetIgnoreCharacterParams() const
{
	FCollisionQueryParams Params;

	TArray<AActor*> CharacterChildren;
	CharacterOwner->GetAllChildActors(CharacterChildren);
	Params.AddIgnoredActors(CharacterChildren);
	Params.AddIgnoredActor(CharacterOwner);

	return Params;
}

#pragma endregion 
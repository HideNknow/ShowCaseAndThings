
#include "SCPlayerCameraManager.h"

void ASCPlayerCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	Super::UpdateViewTarget(OutVT, DeltaTime);
	if (SmoothCamera)
	{
		

		if (AShowCaseAndThingsCharacter* PlayerCharacter = Cast<AShowCaseAndThingsCharacter>(GetOwningPlayerController()->GetPawn()))
		{
			UCustomMovementComponent* ZMC = PlayerCharacter->GetPlayerCharacterMovement();
			FVector TargetCrouchOffset = FVector(
				0,
				0,
				ZMC->GetCrouchedHalfHeight() - PlayerCharacter->GetClass()->GetDefaultObject<ACharacter>()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
			FVector Offset = FMath::Lerp(FVector::ZeroVector, TargetCrouchOffset, FMath::Clamp(CrouchBlendTime / CrouchBlendDuration, 0.f, 1.f));

			if (ZMC->IsCrouching())
			{
				CrouchBlendTime = FMath::Clamp(CrouchBlendTime + DeltaTime, 0.f, CrouchBlendDuration);
				Offset -= TargetCrouchOffset;
			}
			else
			{
				CrouchBlendTime = FMath::Clamp(CrouchBlendTime - DeltaTime, 0.f, CrouchBlendDuration);
			}

			OutVT.POV.Location += Offset;
		}
	}
}

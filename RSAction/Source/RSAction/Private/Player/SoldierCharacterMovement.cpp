// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "RSAction.h"
#include "Player/SoldierCharacterMovement.h"

//----------------------------------------------------------------------//
// UPawnMovementComponent
//----------------------------------------------------------------------//
USoldierCharacterMovement::USoldierCharacterMovement(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


float USoldierCharacterMovement::GetMaxSpeed() const
{
	float MaxSpeed = Super::GetMaxSpeed();

	const ASoldierCharacter* SoldierCharacterOwner = Cast<ASoldierCharacter>(PawnOwner);
	if (SoldierCharacterOwner)
	{
		if (SoldierCharacterOwner->IsTargeting())
		{
			MaxSpeed *= SoldierCharacterOwner->GetTargetingSpeedModifier();
		}
		if (SoldierCharacterOwner->IsRunning())
		{
			MaxSpeed *= SoldierCharacterOwner->GetRunningSpeedModifier();
		}
	}

	return MaxSpeed;
}

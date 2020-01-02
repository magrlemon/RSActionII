// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SoldierPickup.h"
#include "SoldierPickup_Health.generated.h"

class ASoldierCharacter;

// A pickup object that replenishes character health
UCLASS(Abstract, Blueprintable)
class ASoldierPickup_Health : public ASoldierPickup
{
	GENERATED_UCLASS_BODY()

	/** check if pawn can use this pickup */
	virtual bool CanBePickedUp(ASoldierCharacter* TestPawn) const override;

protected:

	/** how much health does it give? */
	UPROPERTY(EditDefaultsOnly, Category=Pickup)
	int32 Health;

	/** give pickup */
	virtual void GivePickupTo(ASoldierCharacter* Pawn) override;
};

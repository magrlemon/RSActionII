// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SoldierPickup.h"
#include "SoldierPickup_Ammo.generated.h"

class ASoldierCharacter;
class ASoldierWeapon;

// A pickup object that replenishes ammunition for a weapon
UCLASS(Abstract, Blueprintable)
class ASoldierPickup_Ammo : public ASoldierPickup
{
	GENERATED_UCLASS_BODY()

	/** check if pawn can use this pickup */
	virtual bool CanBePickedUp(ASoldierCharacter* TestPawn) const override;

	bool IsForWeapon(UClass* WeaponClass);

protected:

	/** how much ammo does it give? */
	UPROPERTY(EditDefaultsOnly, Category=Pickup)
	int32 AmmoClips;

	/** which weapon gets ammo? */
	UPROPERTY(EditDefaultsOnly, Category=Pickup)
	TSubclassOf<ASoldierWeapon> WeaponType;

	/** give pickup */
	virtual void GivePickupTo(ASoldierCharacter* Pawn) override;
};

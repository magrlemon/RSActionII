// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "SoldierSpectatorPawn.generated.h"


UCLASS(config = Game, Blueprintable, BlueprintType)
class ASoldierSpectatorPawn : public ASpectatorPawn
{
	GENERATED_UCLASS_BODY()

	// Begin ASpectatorPawn overrides
	/** Overridden to implement Key Bindings the match the player controls */
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	// End Pawn overrides
	
	// Frame rate linked look
	void LookUpAtRate(float Val);
};

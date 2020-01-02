// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SoldierGame_FreeForAll.generated.h"

class ASoldierPlayerState;

UCLASS()
class ASoldierGame_FreeForAll : public ASoldierGameMode
{
	GENERATED_UCLASS_BODY()

protected:

	/** best player */
	UPROPERTY(transient)
	ASoldierPlayerState* WinnerPlayerState;

	/** check who won */
	virtual void DetermineMatchWinner() override;

	/** check if PlayerState is a winner */
	virtual bool IsWinner(ASoldierPlayerState* PlayerState) const override;
};

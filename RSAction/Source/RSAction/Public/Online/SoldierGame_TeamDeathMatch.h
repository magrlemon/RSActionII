// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SoldierGame_TeamDeathMatch.generated.h"

class ASoldierPlayerState;
class ASoldierAIController;

UCLASS()
class ASoldierGame_TeamDeathMatch : public ASoldierGameMode
{
	GENERATED_UCLASS_BODY()

	/** setup team changes at player login */
	void PostLogin(APlayerController* NewPlayer) override;

	/** initialize replicated game data */
	virtual void InitGameState() override;

	/** can players damage each other? */
	virtual bool CanDealDamage(ASoldierPlayerState* DamageInstigator, ASoldierPlayerState* DamagedPlayer) const override;

protected:

	/** number of teams */
	int32 NumTeams;

	/** best team */
	int32 WinnerTeam;

	/** pick team with least players in or random when it's equal */
	int32 ChooseTeam(ASoldierPlayerState* ForPlayerState) const;

	/** check who won */
	virtual void DetermineMatchWinner() override;

	/** check if PlayerState is a winner */
	virtual bool IsWinner(ASoldierPlayerState* PlayerState) const override;

	/** check team constraints */
	virtual bool IsSpawnpointAllowed(APlayerStart* SpawnPoint, AController* Player) const;

	/** initialization for bot after spawning */
	virtual void InitBot(ASoldierAIController* AIC, int32 BotNum) override;	
};

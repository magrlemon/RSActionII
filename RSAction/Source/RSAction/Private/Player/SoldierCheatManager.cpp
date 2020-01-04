// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "RSAction.h"
#include "Player/SoldierCheatManager.h"
#include "Online/SoldierPlayerState.h"
#include "Bots/SoldierAIController.h"

USoldierCheatManager::USoldierCheatManager(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void USoldierCheatManager::ToggleInfiniteAmmo()
{
	ASoldierPlayerController* MyPC = GetOuterASoldierPlayerController();

	MyPC->SetInfiniteAmmo(!MyPC->HasInfiniteAmmo());
	MyPC->ClientMessage(FString::Printf(TEXT("Infinite ammo: %s"), MyPC->HasInfiniteAmmo() ? TEXT("ENABLED") : TEXT("off")));
}

void USoldierCheatManager::ToggleInfiniteClip()
{
	ASoldierPlayerController* MyPC = GetOuterASoldierPlayerController();

	MyPC->SetInfiniteClip(!MyPC->HasInfiniteClip());
	MyPC->ClientMessage(FString::Printf(TEXT("Infinite clip: %s"), MyPC->HasInfiniteClip() ? TEXT("ENABLED") : TEXT("off")));
}

void USoldierCheatManager::ToggleMatchTimer()
{
	ASoldierPlayerController* MyPC = GetOuterASoldierPlayerController();

	ASoldierGameState* const MyGameState = MyPC->GetWorld()->GetGameState<ASoldierGameState>();
	if (MyGameState && MyGameState->GetLocalRole() == ROLE_Authority)
	{
		MyGameState->bTimerPaused = !MyGameState->bTimerPaused;
		MyPC->ClientMessage(FString::Printf(TEXT("Match timer: %s"), MyGameState->bTimerPaused ? TEXT("PAUSED") : TEXT("running")));
	}
}

void USoldierCheatManager::ForceMatchStart()
{
	ASoldierPlayerController* const MyPC = GetOuterASoldierPlayerController();

	ASoldierGameMode* const MyGame = MyPC->GetWorld()->GetAuthGameMode<ASoldierGameMode>();
	if (MyGame && MyGame->GetMatchState() == MatchState::WaitingToStart)
	{
		MyGame->StartMatch();
	}
}

void USoldierCheatManager::ChangeTeam(int32 NewTeamNumber)
{
	ASoldierPlayerController* MyPC = GetOuterASoldierPlayerController();

	ASoldierPlayerState* MyPlayerState = Cast<ASoldierPlayerState>(MyPC->PlayerState);
	if (MyPlayerState && MyPlayerState->GetLocalRole() == ROLE_Authority)
	{
		MyPlayerState->SetTeamNum(NewTeamNumber);
		MyPC->ClientMessage(FString::Printf(TEXT("Team changed to: %d"), MyPlayerState->GetTeamNum()));
	}
}

void USoldierCheatManager::Cheat(const FString& Msg)
{
	GetOuterASoldierPlayerController()->ServerCheat(Msg.Left(128));
}

void USoldierCheatManager::SpawnBot()
{
	ASoldierPlayerController* const MyPC = GetOuterASoldierPlayerController();
	APawn* const MyPawn = MyPC->GetPawn();
	ASoldierGameMode* const MyGame = MyPC->GetWorld()->GetAuthGameMode<ASoldierGameMode>();
	UWorld* World = MyPC->GetWorld();
	if (MyPawn && MyGame && World)
	{
		static int32 CheatBotNum = 50;
		ASoldierAIController* SoldierAIController = MyGame->CreateBot(CheatBotNum++);
		MyGame->RestartPlayer(SoldierAIController);		
	}
}
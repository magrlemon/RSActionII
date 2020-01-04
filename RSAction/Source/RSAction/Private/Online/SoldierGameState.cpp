// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
#include "Online/SoldierPlayerState.h"
#include "RSAction.h"
#include "ShooterGameInstance.h"

ASoldierGameState::ASoldierGameState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NumTeams = 0;
	RemainingTime = 0;
	bTimerPaused = false;
}

void ASoldierGameState::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( ASoldierGameState, NumTeams );
	DOREPLIFETIME( ASoldierGameState, RemainingTime );
	DOREPLIFETIME( ASoldierGameState, bTimerPaused );
	DOREPLIFETIME( ASoldierGameState, TeamScores );
}

void ASoldierGameState::GetRankedMap(int32 TeamIndex, RankedPlayerMap& OutRankedMap) const
{
	OutRankedMap.Empty();

	//first, we need to go over all the PlayerStates, grab their score, and rank them
	TMultiMap<int32, ASoldierPlayerState*> SortedMap;
	for(int32 i = 0; i < PlayerArray.Num(); ++i)
	{
		int32 Score = 0;
		ASoldierPlayerState* CurPlayerState = Cast<ASoldierPlayerState>(PlayerArray[i]);
		if (CurPlayerState && (CurPlayerState->GetTeamNum() == TeamIndex))
		{
			SortedMap.Add(FMath::TruncToInt(CurPlayerState->Score), CurPlayerState);
		}
	}

	//sort by the keys
	SortedMap.KeySort(TGreater<int32>());

	//now, add them back to the ranked map
	OutRankedMap.Empty();

	int32 Rank = 0;
	for(TMultiMap<int32, ASoldierPlayerState*>::TIterator It(SortedMap); It; ++It)
	{
		OutRankedMap.Add(Rank++, It.Value());
	}
	
}


void ASoldierGameState::RequestFinishAndExitToMainMenu()
{
	if (AuthorityGameMode)
	{
		// we are server, tell the gamemode
		ASoldierGameMode* const GameMode = Cast<ASoldierGameMode>(AuthorityGameMode);
		if (GameMode)
		{
			GameMode->RequestFinishAndExitToMainMenu();
		}
	}
	else
	{
		// we are client, handle our own business
		UShooterGameInstance* GameInstance = Cast<UShooterGameInstance>(GetGameInstance());
		if (GameInstance)
		{
			GameInstance->RemoveSplitScreenPlayers();
		}

		ASoldierPlayerController* const PrimaryPC = Cast<ASoldierPlayerController>(GetGameInstance()->GetFirstLocalPlayerController());
		if (PrimaryPC)
		{
			PrimaryPC->HandleReturnToMainMenu();
		}
	}

}

// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SoldierGame.h"
#include "Online/SoldierPlayerState.h"
#include "Net/OnlineEngineInterface.h"

ASoldierPlayerState::ASoldierPlayerState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	TeamNumber = 0;
	NumKills = 0;
	NumDeaths = 0;
	NumBulletsFired = 0;
	NumRocketsFired = 0;
	bQuitter = false;
}

void ASoldierPlayerState::Reset()
{
	Super::Reset();
	
	//PlayerStates persist across seamless travel.  Keep the same teams as previous match.
	//SetTeamNum(0);
	NumKills = 0;
	NumDeaths = 0;
	NumBulletsFired = 0;
	NumRocketsFired = 0;
	bQuitter = false;
}

void ASoldierPlayerState::RegisterPlayerWithSession(bool bWasFromInvite)
{
	if (UOnlineEngineInterface::Get()->DoesSessionExist(GetWorld(), NAME_GameSession))
	{
		Super::RegisterPlayerWithSession(bWasFromInvite);
	}
}

void ASoldierPlayerState::UnregisterPlayerWithSession()
{
	if (!bFromPreviousLevel && UOnlineEngineInterface::Get()->DoesSessionExist(GetWorld(), NAME_GameSession))
	{
		Super::UnregisterPlayerWithSession();
	}
}

void ASoldierPlayerState::ClientInitialize(AController* InController)
{
	Super::ClientInitialize(InController);

	UpdateTeamColors();
}

void ASoldierPlayerState::SetTeamNum(int32 NewTeamNumber)
{
	TeamNumber = NewTeamNumber;

	UpdateTeamColors();
}

void ASoldierPlayerState::OnRep_TeamColor()
{
	UpdateTeamColors();
}

void ASoldierPlayerState::AddBulletsFired(int32 NumBullets)
{
	NumBulletsFired += NumBullets;
}

void ASoldierPlayerState::AddRocketsFired(int32 NumRockets)
{
	NumRocketsFired += NumRockets;
}

void ASoldierPlayerState::SetQuitter(bool bInQuitter)
{
	bQuitter = bInQuitter;
}

void ASoldierPlayerState::CopyProperties(APlayerState* PlayerState)
{	
	Super::CopyProperties(PlayerState);

	ASoldierPlayerState* SoldierPlayer = Cast<ASoldierPlayerState>(PlayerState);
	if (SoldierPlayer)
	{
		SoldierPlayer->TeamNumber = TeamNumber;
	}	
}

void ASoldierPlayerState::UpdateTeamColors()
{
	AController* OwnerController = Cast<AController>(GetOwner());
	if (OwnerController != NULL)
	{
		ASoldierCharacter* SoldierCharacter = Cast<ASoldierCharacter>(OwnerController->GetCharacter());
		if (SoldierCharacter != NULL)
		{
			SoldierCharacter->UpdateTeamColorsAllMIDs();
		}
	}
}

int32 ASoldierPlayerState::GetTeamNum() const
{
	return TeamNumber;
}

int32 ASoldierPlayerState::GetKills() const
{
	return NumKills;
}

int32 ASoldierPlayerState::GetDeaths() const
{
	return NumDeaths;
}

float ASoldierPlayerState::GetScore() const
{
	return Score;
}

int32 ASoldierPlayerState::GetNumBulletsFired() const
{
	return NumBulletsFired;
}

int32 ASoldierPlayerState::GetNumRocketsFired() const
{
	return NumRocketsFired;
}

bool ASoldierPlayerState::IsQuitter() const
{
	return bQuitter;
}

void ASoldierPlayerState::ScoreKill(ASoldierPlayerState* Victim, int32 Points)
{
	NumKills++;
	ScorePoints(Points);
}

void ASoldierPlayerState::ScoreDeath(ASoldierPlayerState* KilledBy, int32 Points)
{
	NumDeaths++;
	ScorePoints(Points);
}

void ASoldierPlayerState::ScorePoints(int32 Points)
{
	ASoldierGameState* const MyGameState = GetWorld()->GetGameState<ASoldierGameState>();
	if (MyGameState && TeamNumber >= 0)
	{
		if (TeamNumber >= MyGameState->TeamScores.Num())
		{
			MyGameState->TeamScores.AddZeroed(TeamNumber - MyGameState->TeamScores.Num() + 1);
		}

		MyGameState->TeamScores[TeamNumber] += Points;
	}

	Score += Points;
}

void ASoldierPlayerState::InformAboutKill_Implementation(class ASoldierPlayerState* KillerPlayerState, const UDamageType* KillerDamageType, class ASoldierPlayerState* KilledPlayerState)
{
	//id can be null for bots
	if (KillerPlayerState->UniqueId.IsValid())
	{	
		//search for the actual killer before calling OnKill()	
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{		
			ASoldierPlayerController* TestPC = Cast<ASoldierPlayerController>(*It);
			if (TestPC && TestPC->IsLocalController())
			{
				// a local player might not have an ID if it was created with CreateDebugPlayer.
				ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(TestPC->Player);
				FUniqueNetIdRepl LocalID = LocalPlayer->GetCachedUniqueNetId();
				if (LocalID.IsValid() &&  *LocalPlayer->GetCachedUniqueNetId() == *KillerPlayerState->UniqueId)
				{			
					TestPC->OnKill();
				}
			}
		}
	}
}

void ASoldierPlayerState::BroadcastDeath_Implementation(class ASoldierPlayerState* KillerPlayerState, const UDamageType* KillerDamageType, class ASoldierPlayerState* KilledPlayerState)
{	
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		// all local players get death messages so they can update their huds.
		ASoldierPlayerController* TestPC = Cast<ASoldierPlayerController>(*It);
		if (TestPC && TestPC->IsLocalController())
		{
			TestPC->OnDeathMessage(KillerPlayerState, this, KillerDamageType);				
		}
	}	
}

void ASoldierPlayerState::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( ASoldierPlayerState, TeamNumber );
	DOREPLIFETIME( ASoldierPlayerState, NumKills );
	DOREPLIFETIME( ASoldierPlayerState, NumDeaths );
}

FString ASoldierPlayerState::GetShortPlayerName() const
{
	if( GetPlayerName().Len() > MAX_PLAYER_NAME_LENGTH )
	{
		return GetPlayerName().Left(MAX_PLAYER_NAME_LENGTH) + "...";
	}
	return GetPlayerName();
}

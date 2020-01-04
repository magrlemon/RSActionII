// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "RSAction.h"
#include "SoldierHUDPCTrackerBase.h"


/** Initialize with a world context. */
void SoldierHUDPCTrackerBase::Init( const FLocalPlayerContext& InContext )
{
	Context = InContext;
}

TWeakObjectPtr<ASoldierPlayerController> SoldierHUDPCTrackerBase::GetPlayerController() const
{
	if ( ensureMsgf( Context.IsValid(), TEXT("Game context must be initialized!") ) )
	{
		APlayerController* PC = Context.GetPlayerController();
		ASoldierPlayerController* SoldierPC = Cast<ASoldierPlayerController>(PC);
		return MakeWeakObjectPtr(SoldierPC);
	}
	else
	{
		return NULL;
	}
}


UWorld* SoldierHUDPCTrackerBase::GetWorld() const
{
	if ( ensureMsgf( Context.IsValid(), TEXT("Game context must be initialized!") ) )
	{
		return Context.GetWorld();
	}
	else
	{
		return NULL;
	}
}

ASoldierGameState* SoldierHUDPCTrackerBase::GetGameState() const
{
	if ( ensureMsgf( Context.IsValid(), TEXT("Game context must be initialized!") ) )
	{
		return Context.GetWorld()->GetGameState<ASoldierGameState>();
	}
	else
	{
		return NULL;
	}
}

const FLocalPlayerContext& SoldierHUDPCTrackerBase::GetContext() const
{
	return Context;
}




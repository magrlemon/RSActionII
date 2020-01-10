// Copyright 1998-2019 Epic Games, Inc.All Rights Reserved.
#include "SoldierActControllerListenServerQuickMatchClient.h"
#include "SoldierGameSession.h"

void USoldierActControllerListenServerQuickMatchClient::OnTick(float TimeDelta)
{
	Super::OnTick(TimeDelta);

	if (bIsLoggedIn && !bInQuickMatchSearch && !bFoundQuickMatchGame)
	{
		StartQuickMatch();
	}
}
// Copyright 1998-2019 Epic Games, Inc.All Rights Reserved.
#include "SoldierActControllerDedicatedServerTest.h"
#include "SoldierGameSession.h"

void USoldierActControllerDedicatedServerTest::OnTick(float TimeDelta)
{
	Super::OnTick(TimeDelta);

	if (bIsLoggedIn && !bIsSearchingForGame && !bFoundGame)
	{
		StartSearchingForGame();
	}

	if (bIsSearchingForGame && !bFoundGame)
	{
		UpdateSearchStatus();
	}
}
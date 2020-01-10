// Copyright 1998-2019 Epic Games, Inc.All Rights Reserved.
#include "SoldierAct/SoldierActControllerBootTest.h"
#include "SoldierGameInstance.h"

bool USoldierActControllerBootTest::IsBootProcessComplete() const
{
	static double StartTime = FPlatformTime::Seconds();
	const double TimeSinceStart = FPlatformTime::Seconds() - StartTime;

	if (TimeSinceStart >= TestDelay)
	{
		if (const UWorld* World = GetWorld())
		{
			if (const USoldierGameInstance* GameInstance = Cast<USoldierGameInstance>(GetWorld()->GetGameInstance()))
			{
				if (GameInstance->GetCurrentState() == SoldierGameInstanceState::WelcomeScreen ||
					GameInstance->GetCurrentState() == SoldierGameInstanceState::MainMenu)
				{
					return true;
				}
			}
		}
	}

	return false;
}
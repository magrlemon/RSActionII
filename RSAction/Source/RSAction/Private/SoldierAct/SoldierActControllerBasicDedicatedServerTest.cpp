// Copyright 1998-2019 Epic Games, Inc.All Rights Reserved.
#include "SoldierActControllerBasicDedicatedServerTest.h"
#include "SoldierGameInstance.h"

void USoldierActControllerBasicDedicatedServerTest::OnTick(float TimeDelta)
{
	if (GetTimeInCurrentState() > 300)
	{
		UE_LOG(LogGauntlet, Error, TEXT("Failing boot test after 300 secs!"));
		EndTest(-1);
	}
}

void USoldierActControllerBasicDedicatedServerTest::OnPostMapChange(UWorld* World)
{
	if (IsInGame())
	{
		EndTest(0);
	}
}

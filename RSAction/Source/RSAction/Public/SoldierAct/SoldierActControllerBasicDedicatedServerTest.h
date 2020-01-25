// Copyright 1998-2019 Epic Games, Inc.All Rights Reserved.
#pragma once

#include "SoldierAct/SoldierActControllerBase.h"
#include "SoldierActControllerBasicDedicatedServerTest.generated.h"

UCLASS()
class USoldierActControllerBasicDedicatedServerTest : public USoldierActControllerBase
{
	GENERATED_BODY()

protected:
	virtual void OnTick(float TimeDelta) override;

public:
	virtual void OnPostMapChange(UWorld* World) override;
};
// Copyright 1998-2019 Epic Games, Inc.All Rights Reserved.
#pragma once

#include "SoldierActControllerBase.h"
#include "SoldierActControllerListenServerQuickMatchClient.generated.h"

UCLASS()
class USoldierActControllerListenServerQuickMatchClient : public USoldierActControllerBase
{
	GENERATED_BODY()

protected:
	virtual void OnTick(float TimeDelta) override;
};
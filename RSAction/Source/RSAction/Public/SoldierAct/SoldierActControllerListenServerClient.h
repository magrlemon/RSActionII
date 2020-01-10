// Copyright 1998-2019 Epic Games, Inc.All Rights Reserved.
#pragma once

#include "SoldierActControllerBase.h"
#include "SoldierActControllerListenServerClient.generated.h"

UCLASS()
class USoldierActControllerListenServerClient : public USoldierActControllerBase
{
	GENERATED_BODY()

protected:
	virtual void OnTick(float TimeDelta) override;
};
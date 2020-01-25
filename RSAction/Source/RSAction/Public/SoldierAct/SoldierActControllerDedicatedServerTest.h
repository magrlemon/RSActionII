// Copyright 1998-2019 Epic Games, Inc.All Rights Reserved.
#pragma once

#include "SoldierActControllerBase.h"
#include "SharedPointer.h"
#include "SoldierActControllerDedicatedServerTest.generated.h"

UCLASS()
class USoldierActControllerDedicatedServerTest : public USoldierActControllerBase
{
	GENERATED_BODY()

protected:
	virtual void OnTick(float TimeDelta) override;
};
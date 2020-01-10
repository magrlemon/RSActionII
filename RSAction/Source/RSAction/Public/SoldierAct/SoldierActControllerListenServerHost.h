// Copyright 1998-2019 Epic Games, Inc.All Rights Reserved.
#pragma once

#include "SoldierActControllerBase.h"
#include "SoldierActControllerListenServerHost.generated.h"

UCLASS()
class USoldierActControllerListenServerHost : public USoldierActControllerBase
{
	GENERATED_BODY()

public:
	virtual void OnPostMapChange(UWorld* World) override {}

protected:
	virtual void OnUserCanPlayOnline(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults) override;
};
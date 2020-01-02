// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SoldierTypes.h"
#include "SoldierPlayerController_Menu.generated.h"

UCLASS()
class ASoldierPlayerController_Menu : public APlayerController
{
	GENERATED_UCLASS_BODY()

	/** After game is initialized */
	virtual void PostInitializeComponents() override;
};


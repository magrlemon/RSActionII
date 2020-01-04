// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SoldierGame.h"
#include "Player/SoldierPlayerController_Menu.h"
#include "SoldierStyle.h"


ASoldierPlayerController_Menu::ASoldierPlayerController_Menu(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void ASoldierPlayerController_Menu::PostInitializeComponents() 
{
	Super::PostInitializeComponents();

	FSoldierStyle::Initialize();
}

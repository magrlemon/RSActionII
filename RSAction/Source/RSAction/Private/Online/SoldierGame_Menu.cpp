// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
#include "Online/SoldierGame_Menu.h"
#include "SoldierGame.h"

#include "SoldierMainMenu.h"
#include "SoldierWelcomeMenu.h"
#include "SoldierMessageMenu.h"
#include "Player/SoldierPlayerController_Menu.h"
#include "Online/SoldierGameSession.h"

ASoldierGame_Menu::ASoldierGame_Menu(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PlayerControllerClass = ASoldierPlayerController_Menu::StaticClass();
}

void ASoldierGame_Menu::RestartPlayer(class AController* NewPlayer)
{
	// don't restart
}

/** Returns game session class to use */
TSubclassOf<AGameSession> ASoldierGame_Menu::GetGameSessionClass() const
{
	return ASoldierGameSession::StaticClass();
}

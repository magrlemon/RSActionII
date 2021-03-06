// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "RSAction.h"
#include "SoldierGameDelegates.h"

#include "SoldierMenuSoundsWidgetStyle.h"
#include "SoldierMenuWidgetStyle.h"
#include "SoldierMenuItemWidgetStyle.h"
#include "SoldierOptionsWidgetStyle.h"
#include "SoldierScoreboardWidgetStyle.h"
#include "SoldierChatWidgetStyle.h"
#include "AssetRegistryModule.h"
#include "IAssetRegistry.h"



#include "UI/Style/SoldierStyle.h"


class FSoldierGameModule : public FDefaultGameModuleImpl
{
	virtual void StartupModule() override
	{
		InitializeSoldierGameDelegates();
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

		//Hot reload hack
		FSlateStyleRegistry::UnRegisterSlateStyle(FSoldierStyle::GetStyleSetName());
		FSoldierStyle::Initialize();
	}

	virtual void ShutdownModule() override
	{
		FSoldierStyle::Shutdown();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FSoldierGameModule, RSAction, "红海行动");

DEFINE_LOG_CATEGORY(LogShooter)
DEFINE_LOG_CATEGORY(LogShooterWeapon)

// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#ifndef __SOLDIERGAMELOADINGSCREEN_H__
#define __SOLDIERGAMELOADINGSCREEN_H__

#include "ModuleInterface.h"


/** Module interface for this game's loading screens */
class ISoldierGameLoadingScreenModule : public IModuleInterface
{
public:
	/** Kicks off the loading screen for in game loading (not startup) */
	virtual void StartInGameLoadingScreen() = 0;
};

#endif // __SHOOTERGAMELOADINGSCREEN_H__
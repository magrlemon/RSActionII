// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(DcxVehicleLog, Log, All);

class IDcxVehicleModule : public IModuleInterface
{
public:

	static inline IDcxVehicleModule& Get()
	{
		return FModuleManager::LoadModuleChecked<IDcxVehicleModule>("DcxVehicle");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("DcxVehicle");
	}
	
};


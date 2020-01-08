// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "ModuleManager.h"

class IDcxVehicleEditorModule : public IModuleInterface
{
public:

	static inline IDcxVehicleEditorModule& Get()
	{
		return FModuleManager::LoadModuleChecked<IDcxVehicleEditorModule>("DcxVehicleEditor");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("DcxVehicleEditor");
	}

};
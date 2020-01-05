// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ECSystemPlugin.h"
#include "API.h"
#define LOCTEXT_NAMESPACE "FECSystemPluginModule"

void FECSystemPluginModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	ECS::Initialize();
}

void FECSystemPluginModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	ECS::Terminate();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FECSystemPluginModule, ECSystemPlugin)
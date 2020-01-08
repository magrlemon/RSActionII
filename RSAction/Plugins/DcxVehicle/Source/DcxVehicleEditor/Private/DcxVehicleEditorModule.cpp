// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehicleEditorPrivatePCH.h"

class FDcxVehicleEditorModule : public IDcxVehicleEditorModule
{
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE(FDcxVehicleEditorModule, DcxVehicleEditor)

void FDcxVehicleEditorModule::StartupModule()
{
}

void FDcxVehicleEditorModule::ShutdownModule()
{
}
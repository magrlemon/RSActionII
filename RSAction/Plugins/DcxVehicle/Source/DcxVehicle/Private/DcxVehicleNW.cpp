// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "DcxVehicleDriveComponentNW.h"
#include "DcxVehicleNW.h"

ADcxVehicleNW::ADcxVehicleNW(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	VehicleWheels = CreateDefaultSubobject<UDcxVehicleDriveComponentNW>(TEXT("VehicleDriveComponentNW"));
	VehicleWheels->SetIsReplicated(true);
	VehicleWheels->UpdatedComponent = Mesh;
}

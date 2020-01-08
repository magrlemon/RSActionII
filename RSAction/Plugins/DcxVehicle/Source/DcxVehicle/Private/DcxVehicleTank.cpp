// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "DcxVehicleDriveComponentTank.h"
#include "DcxVehicleTank.h"

ADcxVehicleTank::ADcxVehicleTank(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	VehicleWheels = CreateDefaultSubobject<UDcxVehicleDriveComponentTank>(TEXT("VehicleDriveComponentTank"));
	VehicleWheels->SetIsReplicated(true);
	VehicleWheels->UpdatedComponent = Mesh;
}
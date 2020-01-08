// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "DcxVehicleNoDriveComponent.h"
#include "DcxVehicleNoDrive.h"

ADcxVehicleNoDrive::ADcxVehicleNoDrive(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	VehicleWheels = CreateDefaultSubobject<UDcxVehicleNoDriveComponent>(TEXT("VehicleNoDriveComponent"));
	VehicleWheels->SetIsReplicated(true);
	VehicleWheels->UpdatedComponent = Mesh;
}

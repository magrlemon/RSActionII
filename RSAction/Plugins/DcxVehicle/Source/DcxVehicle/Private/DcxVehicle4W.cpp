// Copyright 2016 Doteks Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "DcxVehicleDriveComponent4W.h"
#include "DcxVehicle4W.h"

ADcxVehicle4W::ADcxVehicle4W(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	VehicleWheels = CreateDefaultSubobject<UDcxVehicleDriveComponent4W>(TEXT("VehicleDriveComponent4W"));
	VehicleWheels->SetIsReplicated(true);
	VehicleWheels->UpdatedComponent = Mesh;
}


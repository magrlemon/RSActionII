// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "DcxVehicleDriveInputTank.h"

UDcxVehicleDriveInputTank::UDcxVehicleDriveInputTank(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DriveModel = EDcxVehicleDriveTankControlModel::Standard;
}

float UDcxVehicleDriveInputTank::GetLeftThrust() const
{
	return LeftThrustInput;
}

float UDcxVehicleDriveInputTank::GetRightThrust() const
{
	return RightThrustInput;
}

float UDcxVehicleDriveInputTank::GetThrottle() const
{
	return ThrottleInput;
}

float UDcxVehicleDriveInputTank::GetLeftBrake() const
{
	return LeftBrakeInput;
}

float UDcxVehicleDriveInputTank::GetRightBrake() const
{
	return RightBrakeInput;
}

float UDcxVehicleDriveInputTank::GetClutch() const
{
	return ClutchInput;
}

bool UDcxVehicleDriveInputTank::GetGearUp() const
{
	return bGearUpInput;
}

bool UDcxVehicleDriveInputTank::GetGearDown() const
{
	return bGearDownInput;
}

void UDcxVehicleDriveInputTank::SetLeftThrust(float Thrust)
{
	float Min = DriveModel == EDcxVehicleDriveTankControlModel::Special ? -1.0f : 0.0f;
	LeftThrustInput = FMath::Clamp(Thrust, Min, 1.0f);
}

void UDcxVehicleDriveInputTank::SetRightThrust(float Thrust)
{
	float Min = DriveModel == EDcxVehicleDriveTankControlModel::Special ? -1.0f : 0.0f;
	RightThrustInput = FMath::Clamp(Thrust, Min, 1.0f);
}

void UDcxVehicleDriveInputTank::SetThrottle(float Throttle)
{
	ThrottleInput = FMath::Clamp(Throttle, 0.0f, 1.0f);
}

void UDcxVehicleDriveInputTank::SetLeftBrake(float Brake)
{
	LeftBrakeInput = FMath::Clamp(Brake, 0.0f, 1.0f);
}

void UDcxVehicleDriveInputTank::SetRightBrake(float Brake)
{
	RightBrakeInput = FMath::Clamp(Brake, 0.0f, 1.0f);
}

void UDcxVehicleDriveInputTank::SetClutch(float Clutch)
{
	ClutchInput = FMath::Clamp(Clutch, 0.0f, 1.0f);
}

void UDcxVehicleDriveInputTank::SetGearUp(bool bGearUp)
{
	bGearUpInput = bGearUp;
}

void UDcxVehicleDriveInputTank::SetGearDown(bool bGearDown)
{
	bGearDownInput = bGearDown;
}
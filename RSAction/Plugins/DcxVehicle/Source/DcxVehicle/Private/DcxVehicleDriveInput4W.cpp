// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "DcxVehicleDriveInput4W.h"

UDcxVehicleDriveInput4W::UDcxVehicleDriveInput4W(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SteerInput = 0.0f;
	ThrottleInput = 0.0f;
	BrakeInput = 0.0f;
	ClutchInput = 0.0f;
	HandbrakeInput = 0.0f;
	bGearUpInput = false;
	bGearDownInput = false;
}

float UDcxVehicleDriveInput4W::GetSteer() const
{
	return SteerInput;
}

float UDcxVehicleDriveInput4W::GetThrottle() const
{
	return ThrottleInput;
}

float UDcxVehicleDriveInput4W::GetBrake() const
{
	return BrakeInput;
}

float UDcxVehicleDriveInput4W::GetClutch() const
{
	return ClutchInput;
}

float UDcxVehicleDriveInput4W::GetHandbrake() const
{
	return HandbrakeInput;
}

bool UDcxVehicleDriveInput4W::GetGearUp() const
{
	return bGearUpInput;
}

bool UDcxVehicleDriveInput4W::GetGearDown() const
{
	return bGearDownInput;
}

void UDcxVehicleDriveInput4W::SetSteer(float Steer)
{
	SteerInput = FMath::Clamp(Steer, -1.0f, 1.0f);
}

void UDcxVehicleDriveInput4W::SetThrottle(float Throttle)
{
	ThrottleInput = FMath::Clamp(Throttle, 0.0f, 1.0f);
}

void UDcxVehicleDriveInput4W::SetBrake(float Brake)
{
	BrakeInput = FMath::Clamp(Brake, 0.0f, 1.0f);
}

void UDcxVehicleDriveInput4W::SetClutch(float Clutch)
{
	ClutchInput = FMath::Clamp(Clutch, 0.0f, 1.0f);
}

void UDcxVehicleDriveInput4W::SetHandbrake(float Handbrake)
{
	HandbrakeInput = FMath::Clamp(Handbrake, 0.0f, 1.0f);
}

void UDcxVehicleDriveInput4W::SetGearUp(bool bGearUp)
{
	bGearUpInput = bGearUp;
}

void UDcxVehicleDriveInput4W::SetGearDown(bool bGearDown)
{
	bGearDownInput = bGearDown;
}

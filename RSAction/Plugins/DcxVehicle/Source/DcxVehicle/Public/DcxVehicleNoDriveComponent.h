// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "DcxVehicleWheelsComponent.h"
#include "DcxVehicleNoDriveComponent.generated.h"

UCLASS(ClassGroup = "Dcx|Vehicles")
class DCXVEHICLE_API UDcxVehicleNoDriveComponent : public UDcxVehicleWheelsComponent
{
	GENERATED_UCLASS_BODY()

public:

	/**
	 * Gets the steer angle (degrees) that has been applied to the specified wheel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleNoDriveComponent")
	float GetSteerAngle(int32 WheelIndex) const;

	/**
	 * Set the steer angle to be applied to the specified wheel.
	 * Specified in degrees.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleNoDriveComponent")
	void SetSteerAngle(int32 WheelIndex, float SteerAngle);

	/**
	 * Get the drive torque (Nm) that has been applied to the specified wheel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleNoDriveComponent")
	float GetDriveTorque(int32 WheelIndex) const;

	/**
	 * Set the drive torque to be applied to the specified wheel.
	 * Specified in Newton meters (Nm).
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleNoDriveComponent")
	void SetDriveTorque(int32 WheelIndex, float DriveTorque);

	/**
	 * Get the brake torque (Nm) that has been applied to the specified wheel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleNoDriveComponent")
	float GetBrakeTorque(int32 WheelIndex) const;

	/**
	 * Set the brake torque to be applied to the specified wheel.
	 * Specified in Newton meters (Nm).
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleNoDriveComponent")
	void SetBrakeTorque(int32 WheelIndex, float BrakeTorque);

protected:

	virtual void SetupDrive(physx::PxVehicleWheelsSimData* PWheelsSimData) override;
	virtual void PostDisableWheel(int32 WheelIndex) override;

};
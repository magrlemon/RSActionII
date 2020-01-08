// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "DcxVehicleDriveComponent.h"
#include "DcxVehicleInputRate.h"
#include "DcxVehicleDriveInputNW.h"
#include "DcxVehicleDriveComponentNW.generated.h"

namespace physx
{
	class PxVehicleWheelsSimData;
}

UCLASS(ClassGroup = "Dcx|Vehicle")
class DCXVEHICLE_API UDcxVehicleDriveComponentNW : public UDcxVehicleDriveComponent
{
	GENERATED_UCLASS_BODY()

protected:

	/**
	 * The rate at which input steering can rise and fall.
	 */
	UPROPERTY(EditAnywhere, Category = "Input", AdvancedDisplay)
	FDcxVehicleInputRate SteerRate;

	/**
	 * The rate at which input throttle can rise and fall.
	 */
	UPROPERTY(EditAnywhere, Category = "Input", AdvancedDisplay)
	FDcxVehicleInputRate ThrottleRate;

	/**
	 * The rate at which input brake can rise and fall.
	 */
	UPROPERTY(EditAnywhere, Category = "Input", AdvancedDisplay)
	FDcxVehicleInputRate BrakeRate;

	/**
	 * The rate at which input handbrake can rise and fall.
	 */
	UPROPERTY(EditAnywhere, Category = "Input", AdvancedDisplay)
	FDcxVehicleInputRate HandbrakeRate;

	/**
	 * The graph for maximum steering versus forward speed.
	 */
	UPROPERTY(EditAnywhere, Category = "Steering")
	FRuntimeFloatCurve SteerCurve;

	UPROPERTY(Transient)
	UDcxVehicleDriveInputNW* RawInput;

	UPROPERTY(Transient)
	UDcxVehicleDriveInputNW* RipeInput;

public:

	UFUNCTION(BlueprintCallable, Category = "Dcx|Components|VehicleDriveComponentNW")
	void SetEngineData(FDcxVehicleEngineData& NewEngineData) override;

	UFUNCTION(BlueprintCallable, Category = "Dcx|Components|VehicleDriveComponentNW")
	void SetGearboxData(FDcxVehicleGearboxData& NewGearboxData) override;

	UFUNCTION(BlueprintCallable, Category = "Dcx|Components|VehicleDriveComponentNW")
	void SetClutchData(FDcxVehicleClutchData& NewClutchData) override;

	/**
	 * The vehicle driving control values from analog inputs.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Components|VehicleDriveComponentNW")
	UDcxVehicleDriveInputNW* GetInput() const;

	/**
	 * Test if a specific wheel has been configured as a driven or non-driven wheel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Components|VehicleDriveComponentNW")
	bool IsDrivenWheel(int32 WheelIndex) const;

	/**
	 * Set a specific wheel to be driven or non-driven by the differential.
	 * The available drive torque will be split equally between all driven wheels.
	 * Zero torque will be applied to non-driven wheels.
	 * The default state of each wheel is to be uncoupled to the differential.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Components|VehicleDriveComponentNW")
	void SetDrivenWheel(int32 WheelIndex, bool bIsDriven);

protected:

	virtual bool CanCreateVehicle() const override;
	virtual void SetupDrive(physx::PxVehicleWheelsSimData* PWheelsSimData) override;
	virtual void UpdateState(float DeltaTime) override;
	virtual void UpdateSimulation(float DeltaTime) override;
	virtual void PostDisableWheel(int32 WheelIndex) override;

};
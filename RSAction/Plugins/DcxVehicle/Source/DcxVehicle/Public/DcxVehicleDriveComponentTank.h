// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "DcxVehicleDriveComponent.h"
#include "DcxVehicleInputRate.h"
#include "DcxVehicleDriveInputTank.h"
#include "DcxVehicleDriveComponentTank.generated.h"

namespace physx
{
	class PxVehicleWheelsSimData;
}

UCLASS(ClassGroup = "Dcx|Vehicle")
class DCXVEHICLE_API UDcxVehicleDriveComponentTank : public UDcxVehicleDriveComponent
{
	GENERATED_UCLASS_BODY()

protected:

	/**
	 * The control model used by the tank.
	 * Standard: Turning achieved by braking on one side, accelerating on the other side.
	 * Special: Turning achieved by accelerating forwards on one side, accelerating backwards on the other side.
	 */
	UPROPERTY(EditAnywhere, Category = "Input")
	TEnumAsByte<EDcxVehicleDriveTankControlModel::Type> DriveModel;

	/**
	 * The rate at which input left and right thrust can rise and fall.
	 */
	UPROPERTY(EditAnywhere, Category = "Input", AdvancedDisplay)
	FDcxVehicleInputRate ThrustRate;

	/**
	 * The rate at which input throttle can rise and fall.
	 */
	UPROPERTY(EditAnywhere, Category = "Input", AdvancedDisplay)
	FDcxVehicleInputRate ThrottleRate;

	/**
	 * The rate at which input left and right brake can rise and fall.
	 */
	UPROPERTY(EditAnywhere, Category = "Input", AdvancedDisplay)
	FDcxVehicleInputRate BrakeRate;

	UPROPERTY(Transient)
	UDcxVehicleDriveInputTank* RawInput;

	UPROPERTY(Transient)
	UDcxVehicleDriveInputTank* RipeInput;

public:

	UFUNCTION(BlueprintCallable, Category = "Dcx|Components|VehicleDriveComponentTank")
	void SetEngineData(FDcxVehicleEngineData& NewEngineData) override;

	UFUNCTION(BlueprintCallable, Category = "Dcx|Components|VehicleDriveComponentTank")
	void SetGearboxData(FDcxVehicleGearboxData& NewGearboxData) override;

	UFUNCTION(BlueprintCallable, Category = "Dcx|Components|VehicleDriveComponentTank")
	void SetClutchData(FDcxVehicleClutchData& NewClutchData) override;

	/**
	 * Gets the control model used by the tank.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Components|VehicleDriveComponentTank")
	EDcxVehicleDriveTankControlModel::Type GetDriveModel() const;

	/**
	 * Sets the control model used by the tank.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Components|VehicleDriveComponentTank")
	void SetDriveModel(EDcxVehicleDriveTankControlModel::Type Model);

	/**
	 * The vehicle driving control values from analog inputs.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Components|VehicleDriveComponentTank")
	UDcxVehicleDriveInputTank* GetInput() const;

protected:

	virtual bool CanCreateVehicle() const override;
	virtual void SetupDrive(physx::PxVehicleWheelsSimData* PWheelsSimData) override;
	virtual void UpdateState(float DeltaTime) override;
	virtual void UpdateSimulation(float DeltaTime) override;

};

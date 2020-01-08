// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "DcxVehicleWheelsComponent.h"
#include "DcxVehicleEngineData.h"
#include "DcxVehicleGearboxData.h"
#include "DcxVehicleClutchData.h"
#include "DcxVehicleDriveComponent.generated.h"

namespace physx
{
	class PxVehicleDrive;
}

UCLASS(Abstract, BlueprintType, ClassGroup = "Dcx|Vehicle")
class DCXVEHICLE_API UDcxVehicleDriveComponent : public UDcxVehicleWheelsComponent
{
	GENERATED_UCLASS_BODY()

protected:

	/**
	 * If true, the vehicle will automatically switch to reverse on brake input or vice versa.
	 */
	UPROPERTY(EditAnywhere, Category = "Input")
	bool bAutoReverse;

	/**
	 * Auto-reverse when vehicle forward speed is opposite of player input by at least this much (km/h).
	 */
	UPROPERTY(EditAnywhere, Category = "Input")
	float WrongDirectionThreshold;

	/**
	 * If true, the vehicle will automatically brake when absolute forward speed is less than StopThreshold.
	*/
	UPROPERTY(EditAnywhere, Category = "Input")
	bool bAutoBrake;

	/**
	 * Auto-brake when absolute vehicle forward speed is less than this (km/h).
	 */
	UPROPERTY(EditAnywhere, Category = "Input")
	float StopThreshold;

	/**
	 * Engine data of the vehicle.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Mechanics")
	FDcxVehicleEngineData EngineData;

	/**
	 * Transmission data of the vehicle.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Mechanics")
	FDcxVehicleGearboxData GearboxData;

	/**
	 * Clutch data of the vehicle.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Mechanics")
	FDcxVehicleClutchData ClutchData;

public:

	/**
	 * Gets the engine data.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleDriveComponent")
	FDcxVehicleEngineData GetEngineData() const;

	/**
	 * Sets the engine data.
	 */
	//UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleDriveComponent")
	virtual void SetEngineData(FDcxVehicleEngineData& NewEngineData);

	/**
	 * Gets the engine rotation in revolutions per minute (RPM).
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleDriveComponent")
	float GetEngineRotationSpeed() const;

	/**
	 * Gets the transmission data.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleDriveComponent")
	FDcxVehicleGearboxData GetGearboxData() const;

	/**
	 * Sets the transmission data.
	 */
	//UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleDriveComponent")
	virtual void SetGearboxData(FDcxVehicleGearboxData& NewGearboxData);

	/**
	 * Gets the type of gearbox.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleDriveComponent")
	EDcxVehicleGearbox::Type GetGearboxType() const;

	/**
	 * Sets the type of gearbox.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleDriveComponent")
	void SetGearboxType(EDcxVehicleGearbox::Type NewGearboxType);

	/**
	 * Gets the target gear.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleDriveComponent")
	int32 GetCurrentGear() const;

	/**
	 * Gets the target gear.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleDriveComponent")
	int32 GetTargetGear() const;

	/**
	 * Sets the user input for target gear (-1 Reverse, 0 Neutral, >= 1 Forward).
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveComponent")
	void SetTargetGear(int32 NewGear, bool bImmediate);

	/**
	 * Gets the clutch data.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleDriveComponent")
	FDcxVehicleClutchData GetClutchData() const;
	
	/**
	 * Sets the clutch data.
	 */
	//UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleDriveComponent")
	virtual void SetClutchData(FDcxVehicleClutchData& NewClutchData);

protected:

	virtual void CheckAutoReverse(float& Throttle, int32 NumBrakes, float* Brakes);
	virtual void CheckAutoBrake(float& Throttle, int32 NumBrakes, float* Brakes);
	virtual void UpdateClutch(float ClutchInput, PxVehicleClutchData& PClutchData);

	int32 P2UGear(const uint32 PGear) const;
	uint32 U2PGear(const int32 UGear) const;

};

// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "DcxVehicleDriveInputTank.generated.h"

UENUM(BlueprintType)
namespace EDcxVehicleDriveTankControlModel
{
	enum Type
	{
		Standard = 0,
		Special
	};
}

UCLASS(BlueprintType)
class DCXVEHICLE_API UDcxVehicleDriveInputTank : public UObject
{
	GENERATED_UCLASS_BODY()

	friend class UDcxVehicleDriveComponentTank;

public:

	/**
	 * Gets the left thrust value in range [0.0, 1.0].
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	float GetLeftThrust() const;

	/**
	 * Gets the right thrust value in range [0.0, 1.0].
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	float GetRightThrust() const;

	/**
	 * Gets the throttle value in range [0.0, 1.0].
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	float GetThrottle() const;

	/**
	 * Gets the left brake value in range [0.0, 1.0].
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	float GetLeftBrake() const;

	/**
	 * Gets the right brake value in range [0.0, 1.0].
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	float GetRightBrake() const;

	/**
	 * Return the clutch value in range [0.0, 1.0].
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	float GetClutch() const;

	/**
	 * Gets if the gear up button has been pressed.
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	bool GetGearUp() const;

	/**
	 * Gets if the gear down button has been pressed.
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	bool GetGearDown() const;

	/**
	 * Sets the left thrust value in range [0.0, 1.0].
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	void SetLeftThrust(float Thrust);

	/**
	 * Sets the right thrust value in range [0.0, 1.0].
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	void SetRightThrust(float Thrust);

	/**
	 * Sets the throttle value in range [0.0, 1.0].
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	void SetThrottle(float Throttle);

	/**
	 * Sets the left brake value in range [0.0, 1.0].
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	void SetLeftBrake(float Brake);

	/**
	 * Sets the right brake value in range [0.0, 1.0].
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	void SetRightBrake(float Brake);

	/**
	 * Sets the clutch value in range [0.0, 1.0].
	 * A value of 0.0 means fully retracted and 1.0 means fully engaged.
	 * The clutch input will be ignored if the gearbox is set to Automatic or Semi Automatic.
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	void SetClutch(float Clutch);

	/**
	 * Sets if the gear up button has been pressed.
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	void SetGearUp(bool bGearUp);

	/**
	 * Sets if the gear down button has been pressed.
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	void SetGearDown(bool bGearDown);

protected:

	UPROPERTY(Transient)
	float LeftThrustInput;

	UPROPERTY(Transient)
	float RightThrustInput;

	UPROPERTY(Transient)
	float ThrottleInput;

	UPROPERTY(Transient)
	float LeftBrakeInput;

	UPROPERTY(Transient)
	float RightBrakeInput;

	UPROPERTY(Transient)
	float ClutchInput;

	UPROPERTY(Transient)
	bool bGearUpInput;

	UPROPERTY(Transient)
	bool bGearDownInput;

	UPROPERTY(Transient)
	TEnumAsByte<EDcxVehicleDriveTankControlModel::Type> DriveModel;

};

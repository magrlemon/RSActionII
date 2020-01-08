// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "DcxVehicleDriveInput4W.generated.h"

UCLASS(BlueprintType)
class DCXVEHICLE_API UDcxVehicleDriveInput4W : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	/**
	 * Gets the steer value in range [-1.0, 1.0].
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	float GetSteer() const;

	/**
	 * Gets the throttle value in range [0.0, 1.0].
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	float GetThrottle() const;

	/**
	 * Gets the brake value in range [0.0, 1.0].
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	float GetBrake() const;

	/**
	 * Return the clutch value in range [0.0, 1.0].
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	float GetClutch() const;

	/**
	 * Gets the handbrake value in range [0.0, 1.0].
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	float GetHandbrake() const;

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
	 * Sets the steer value in range [-1.0, 1.0].
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	void SetSteer(float Steer);

	/**
	 * Sets the throttle value in range [0.0, 1.0].
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	void SetThrottle(float Throttle);

	/**
	 * Sets the brake value in range [0.0, 1.0].
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	void SetBrake(float Brake);

	/**
	 * Sets the clutch value in range [0.0, 1.0].
	 * A value of 0.0 means fully retracted and 1.0 means fully engaged.
	 * The clutch input will be ignored if the gearbox is set to Automatic or Semi Automatic.
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	void SetClutch(float Clutch);

	/**
	 * Sets the handbrake value in range [0.0, 1.0].
	 */
	UFUNCTION(BlueprintCallable, Category = "DriveCodex|Vehicle|VehicleDriveInput")
	void SetHandbrake(float Handbrake);

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
	float SteerInput;

	UPROPERTY(Transient)
	float ThrottleInput;

	UPROPERTY(Transient)
	float BrakeInput;

	UPROPERTY(Transient)
	float ClutchInput;

	UPROPERTY(Transient)
	float HandbrakeInput;

	UPROPERTY(Transient)
	bool bGearUpInput;

	UPROPERTY(Transient)
	bool bGearDownInput;

};

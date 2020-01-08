// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "DcxVehicleDriveComponent.h"
#include "DcxVehicleInputRate.h"
#include "DcxVehicleDifferentialData4W.h"
#include "DcxVehicleDriveInput4W.h"
#include "DcxVehicleDriveComponent4W.generated.h"

namespace physx
{
	class PxVehicleWheelsSimData;
}

UENUM(BlueprintType)
namespace EDcxVehicleWheelOrder4W
{
	enum Type
	{
		FrontLeft = 0,
		FrontRight,
		RearLeft,
		RearRight
	};
}

UCLASS(ClassGroup = "Dcx|Vehicle")
class DCXVEHICLE_API UDcxVehicleDriveComponent4W : public UDcxVehicleDriveComponent
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
	 * Differential properties of the vehicle.
	 */
	UPROPERTY(EditAnywhere, Category = "Mechanics")
	FDcxVehicleDifferentialData4W DifferentialData;

	/**
	 * The graph for maximum steering versus forward speed.
	 */
	UPROPERTY(EditAnywhere, Category = "Steering")
	FRuntimeFloatCurve SteerCurve;
	
	/**
	 * Accuracy of Ackermann steer calculation.
	 * Accuracy with value 0.0 results in no Ackermann steer-correction, while
	 * accuracy with value 1.0 results in perfect Ackermann steer-correction.
	 * Perfect Ackermann steer correction modifies the steer angles applied to
	 * the front-left and front-right wheels so that the perpendiculars to the
	 * wheels' longitudinal directions cross the extended vector of the
	 * rear axle at the same point. It is also applied to any steer angle applied
	 * to the the rear wheels but instead using the extended vector of the front axle.
	 * In general, more steer correction produces better cornering behavior.
	 */
	UPROPERTY(EditAnywhere, Category = "Steering")
	float AckermannGeometryAccuracy;

	UPROPERTY(Transient)
	UDcxVehicleDriveInput4W* RawInput;

	UPROPERTY(Transient)
	UDcxVehicleDriveInput4W* RipeInput;

public:

	UFUNCTION(BlueprintCallable, Category = "Dcx|Components|VehicleDriveComponent4W")
	void SetEngineData(FDcxVehicleEngineData& NewEngineData) override;

	UFUNCTION(BlueprintCallable, Category = "Dcx|Components|VehicleDriveComponent4W")
	void SetGearboxData(FDcxVehicleGearboxData& NewGearboxData) override;

	UFUNCTION(BlueprintCallable, Category = "Dcx|Components|VehicleDriveComponent4W")
	void SetClutchData(FDcxVehicleClutchData& NewClutchData) override;

	/**
	 * The vehicle driving control values from analog inputs.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Components|VehicleDriveComponent4W")
	UDcxVehicleDriveInput4W* GetInput() const;

	/**
	 * Gets the differential properties of the vehicle.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Components|VehicleDriveComponent4W")
	FDcxVehicleDifferentialData4W GetDifferentialData() const;

	/**
	 * Sets the differential properties of the vehicle.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Components|VehicleDriveComponent4W")
	void SetDifferentialData(FDcxVehicleDifferentialData4W& NewDifferentialData);
		
protected:

	virtual bool CanCreateVehicle() const override;
	virtual void SetupDrive(physx::PxVehicleWheelsSimData* PWheelsSimData) override;
	virtual void UpdateState(float DeltaTime) override;
	virtual void UpdateSimulation(float DeltaTime) override;
	virtual void PostDisableWheel(int32 WheelIndex) override;
};

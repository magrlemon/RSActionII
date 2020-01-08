// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "DcxVehicleTire.h"
#include "DcxVehicleWheelData.generated.h"

class UStaticMesh;

namespace physx
{
	class PxVehicleWheelData;
}

USTRUCT(BlueprintType)
struct DCXVEHICLE_API FDcxVehicleWheelData
{
	GENERATED_USTRUCT_BODY()

public:

	FDcxVehicleWheelData();

	/**
	 * Radius of unit that includes metal wheel plus rubber tire.
	 * If radius is specified as 0, it will be calculated from mesh.
	 * Specified in centimeters (cm).
	 */
	UPROPERTY(EditAnywhere, Category = "Shape")
	float Radius;

	/**
	 * Maximum width of unit that includes wheel plus tire.
	 * If width is specified as 0, it will be calculated from mesh.
	 * Specified in centimeters (cm).
	 */
	UPROPERTY(EditAnywhere, Category = "Shape")
	float Width;

	/**
	 * Mass of unit that includes wheel plus tire.
	 * Specified in kilograms (kg).
	 */
	UPROPERTY(EditAnywhere, Category = "Wheel")
	float Mass;

	/**
	 * Damping rate applied to wheel.
	 * Specified in kilograms meters-squared per second (kg m^2 s^-1).
	 */
	UPROPERTY(EditAnywhere, Category = "Wheel")
	float DampingRate;

	/**
	 * Max brake torque that can be applied to wheel.
	 * Specified in newton meters (Nm).
	 */
	UPROPERTY(EditAnywhere, Category = "Wheel")
	float MaxBrakeTorque;

	/**
	 * Max handbrake torque that can be applied to wheel.
	 * Specified in newton meters (Nm).
	 */
	UPROPERTY(EditAnywhere, Category = "Wheel")
	float MaxHandBrakeTorque;

	/**
     * Max steer angle that can be achieved by the wheel.
	 * Specified in degrees.
	 */
	UPROPERTY(EditAnywhere, Category = "Wheel")
	float MaxSteer;

	/**
	 * Wheel toe angle.
	 * Specified in degrees.
	 */
	UPROPERTY(EditAnywhere, Category = "Wheel")
	float ToeAngle;

	void Setup(physx::PxVehicleWheelData& PWheelData);

};

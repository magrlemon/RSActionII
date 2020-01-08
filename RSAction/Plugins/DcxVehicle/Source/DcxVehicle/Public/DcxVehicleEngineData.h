// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "DcxVehicleEngineData.generated.h"

namespace physx
{
	class PxVehicleEngineData;
}

USTRUCT(BlueprintType)
struct DCXVEHICLE_API FDcxVehicleEngineData
{
	GENERATED_USTRUCT_BODY()

public:

	FDcxVehicleEngineData();

	/**
	 * Graph of normalized torque against normalized engine speed.
	 * The normalized engine speed is the x-axis of the graph,
	 * while the normalized torque is the y-axis of the graph.
	 */
	UPROPERTY(EditAnywhere, Category = "EngineSetup")
	FRuntimeFloatCurve TorqueCurve;

	/**
	 * Moment of inertia of the engine around the axis of rotation.
	 * Specified in kilograms meters-squared (kg m^2).
	 */
	UPROPERTY(EditAnywhere, Category = "EngineSetup", Meta = (ClampMin = "0.01", UIMin = "0.01"))
	float MOI;

	/**
	 * Maximum revolutions per minute of the engine.
	 */
	UPROPERTY(EditAnywhere, Category = "EngineSetup", Meta = (ClampMin = "0.01", UIMin = "0.01"))
	float MaxRPM;

	/**
	 * Damping rate of engine when full throttle is applied.
	 * Specified in kilograms meters-squared per second (kg m^2 s^-1).
	 */
	UPROPERTY(EditAnywhere, Category = "EngineSetup", AdvancedDisplay, Meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DampingRateFullThrottle;

	/**
	 * Damping rate of engine when full throttle is applied and clutch engaged.
	 * Specified in kilograms metres-squared per second (kg m^2 s^-1).
	 */
	UPROPERTY(EditAnywhere, Category = "EngineSetup", AdvancedDisplay, Meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DampingRateZeroThrottleClutchEngaged;

	/**
	 * Damping rate of engine when full throttle is applied and clutch disengaged.
	 * Specified in kilograms metres-squared per second (kg m^2 s^-1).
	 */
	UPROPERTY(EditAnywhere, Category = "EngineSetup", AdvancedDisplay, Meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DampingRateZeroThrottleClutchDisengaged;

	// Calculates the peak torque from TorqueCurve.
	float GetPeakTorque() const;

	void Setup(physx::PxVehicleEngineData& PEngineData);

};

// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "DcxVehicleWheelState.generated.h"

namespace physx
{
	struct PxWheelQueryResult;
}

USTRUCT(BlueprintType)
struct DCXVEHICLE_API FDcxVehicleWheelState
{
	GENERATED_USTRUCT_BODY()

public:

	FDcxVehicleWheelState();

	/**
	 * Start point of suspension line raycast used in raycast completed immediately before vehicle update.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "WheelState")
	FVector SuspensionLineStart;

	/**
	 * Directions of suspension line raycast used in raycast completed immediately before vehicle update.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "WheelState")
	FVector SuspensionLineDirection;

	/**
	 * Lengths of suspension line raycast used in raycast completed immediately before vehicle update.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "WheelState")
	float SuspensionLineLength;

	/**
	 * If suspension travel limits forbid the wheel from touching the drivable surface then returns true.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "WheelState")
	bool bIsInAir;

	/**
	 * PhysicalMaterial instance of the driving surface under the corresponding vehicle wheel.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "WheelState")
	UPhysicalMaterial* TireSurfaceMaterial;

	/**
	 * Point on the drivable surface hit by the most recent suspension raycast.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "WheelState")
	FVector TireContactPoint;

	/**
	 * Normal on the drivable surface at the hit point of the most recent suspension raycast.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "WheelState")
	FVector TireContactNormal;

	/**
	 * Friction experienced by the tire for the combination of tire type
	 * and surface type after accounting for the friction vs slip graph.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "WheelState")
	float TireFriction;

	/**
	 * Compression of the suspension spring.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "WheelState")
	float SuspensionJounce;

	/**
	 * Magnitude of force applied by the suspension spring along the direction of suspension travel.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "WheelState")
	float SuspensionSpringForce;

	/**
	 * Forward direction of the wheel/tire accounting for steer/toe/camber angle
	 * projected on to the contact plane of the drivable surface.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "WheelState")
	FVector TireLongitudinalDirection;

	/**
	 * Lateral direction of the wheel/tire accounting for steer/toe/camber angle
	 * projected on to the contact plan of the drivable surface.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "WheelState")
	FVector TireLateralDirection;

	/**
	 * Longitudinal slip of the tire.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "WheelState")
	float LongitudinalSlip;

	/**
	 * Lateral slip of the tire.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "WheelState")
	float LateralSlip;

	/**
	 * Steer angle of the wheel about the "up" vector accounting for
	 * input steer and toe and, if applicable, Ackermann steer correction.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "WheelState")
	float SteerAngle;

	/**
	 * Local pose of the wheel.
	 */
	FTransform LocalPose;

	void Setup(physx::PxWheelQueryResult& PQueryResult);

};
// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "DcxVehicleSuspensionData.generated.h"

namespace physx
{
	class PxVehicleSuspensionData;
}

USTRUCT(BlueprintType)
struct DCXVEHICLE_API FDcxVehicleSuspensionData
{
	GENERATED_USTRUCT_BODY()

public:

	FDcxVehicleSuspensionData();

	/**
	 * Oscillation frequency of suspension. Standard cars have values between 5 and 10.
	 */
	UPROPERTY(EditAnywhere, Category = "Suspension")
	float NaturalFrequency;

	/**
	 * The rate at which energy is dissipated from the spring. Standard cars have values between 0.8 and 1.2.
	 * Values < 1 are more sluggish, values > 1 or more twitchy.
	 */
	UPROPERTY(EditAnywhere, Category = "Suspension")
	float SpringDamperRatio;

	/**
	 * Maximum compression allowed by suspension spring.
	 * Specified in centimeters (cm).
	 */
	UPROPERTY(EditAnywhere, Category = "Suspension")
	float MaxCompression;

	/**
	 * Maximum elongation allowed by suspension spring.
	 * Specified in centimeters (cm).
	 */
	UPROPERTY(EditAnywhere, Category = "Suspension")
	float MaxDroop;

	/**
	 * Camber angle of wheel when the suspension is at its rest position.
	 * Specified in degrees.
	 */
	UPROPERTY(EditAnywhere, Category = "Suspension")
	float CamberAtRest;

	/**
	 * Camber angle of wheel when the suspension is at maximum compression.
	 * Specified in degrees.
     */
	UPROPERTY(EditAnywhere, Category = "Suspension")
	float CamberAtMaxCompression;

	/**
	 * Camber angle of wheel when the suspension is at maximum droop.
	 * Specified in degrees.
	 */
	UPROPERTY(EditAnywhere, Category = "Suspension")
	float CamberAtMaxDroop;

	void Setup(physx::PxVehicleSuspensionData& PSuspensionData);

};

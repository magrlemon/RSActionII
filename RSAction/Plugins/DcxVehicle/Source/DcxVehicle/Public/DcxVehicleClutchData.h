// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "DcxVehicleClutchData.generated.h"

namespace physx
{
	class PxVehicleClutchData;
}

UENUM(BlueprintType)
namespace EDcxVehicleClutchAccuracyMode
{
	enum Type
	{
		Estimate = 0,
		BestPossible
	};
}

USTRUCT(BlueprintType)
struct DCXVEHICLE_API FDcxVehicleClutchData
{
	GENERATED_USTRUCT_BODY()

public:

	FDcxVehicleClutchData();

	/**
	 * The clutch bite point in range [0.0, 1.0] at which the torque will be transferred to transmission.
	 * This will be ignored if the gearbox is set to Automatic or Semi Automatic.
	 */
	UPROPERTY(EditAnywhere, Category = "Clutch")
	float BitePoint;

	/**
	 * Strength of clutch.
	 * Specified in kilograms metres-squared per second (kg m^2 s^-1).
	 */
	UPROPERTY(EditAnywhere, Category = "Clutch")
	float Strength;

	/**
	 * The engine and wheel rotation speeds that are coupled through the clutch
	 * can be updatedby choosing one of two modes: Estimate and BestPossible.
	 */
	UPROPERTY(EditAnywhere, Category = "Clutch")
	TEnumAsByte<EDcxVehicleClutchAccuracyMode::Type> AccuracyMode;

	/**
	 * Tune the mathematical accuracy and computational cost of the computed
	 * estimate to the wheel and engine rotation speeds if Estimate is chosen.
	 */
	UPROPERTY(EditAnywhere, Category = "Clutch")
	uint32 EstimateIterations;

	void Setup(physx::PxVehicleClutchData& PClutchData);

};
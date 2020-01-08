// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "DcxVehicleGearboxData.generated.h"

namespace physx
{
	class PxVehicleAutoBoxData;
	class PxVehicleGearsData;
}

UENUM(BlueprintType)
namespace EDcxVehicleGearbox
{
	enum Type
	{
		Automatic,
		SemiAutomatic,
		Manual
	};
}

USTRUCT(BlueprintType)
struct DCXVEHICLE_API FDcxVehicleGearData
{
	GENERATED_USTRUCT_BODY()

public:

	FDcxVehicleGearData();
	FDcxVehicleGearData(float GearRatio, float GearDownRatio, float GearUpRatio);

	/**
	 * Gear ratio, i.e. the amount of torque multiplication.
	 */
	UPROPERTY(EditAnywhere, Category = "Gear")
	float Ratio;

	/**
	 * Value of RPM/MaxRPM that is low enough to decrement gear.
	 */
	UPROPERTY(EditAnywhere, Category = "Gear", Meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
	float DownRatio;

	/**
	 * Value of RPM/MaxRPM that is high enough to increment gear.
	 */
	UPROPERTY(EditAnywhere, Category = "Gear", Meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
	float UpRatio;
};

USTRUCT(BlueprintType)
struct FDcxVehicleGearboxData
{
	GENERATED_USTRUCT_BODY()

public:

	FDcxVehicleGearboxData();

	/**
	 * The type of gearbox.
	 */
	UPROPERTY(EditAnywhere, Category = "Gearbox")
	TEnumAsByte<EDcxVehicleGearbox::Type> Type;

	/**
	 * Gear ratio applied is current gear ratio multiplied by final ratio.
	 */
	UPROPERTY(EditAnywhere, Category = "Gearbox")
	float FinalRatio;

	/**
	 * Time it takes to switch gear.
	 * Specified in seconds (s).
	 */
	UPROPERTY(EditAnywhere, Category = "Gearbox", Meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SwitchTime;

	/**
	 * Forward gear ratios.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Gearbox")
	TArray<FDcxVehicleGearData> ForwardGears;

	/**
	 * Reverse gear ratio.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Gearbox")
	float ReverseGearRatio;

	/**
 	 * The minimum time that must pass between each gear change that is initiated by the autobox.
	 * The auto-box will only attempt to initiate another gear change up or down if the simulation time
	 * that has passed since the most recent automated gear change is greater than the specified latency.
	 * Specified in seconds (s).
	 */
	UPROPERTY(EditAnywhere, Category = "AutoGearbox")
	float Latency;

	/**
	 * Value of RPM/MaxRPM for neutral gear that is high enough to switch to first gear.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "AutoGearbox", Meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
	float NeutralGearUpRatio;

	void Setup(physx::PxVehicleGearsData& PGearsData, physx::PxVehicleAutoBoxData& PAutoBoxData);

};

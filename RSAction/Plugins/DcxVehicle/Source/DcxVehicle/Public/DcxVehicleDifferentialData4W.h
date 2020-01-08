// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "DcxVehicleDifferentialData4W.generated.h"

namespace physx
{
	class PxVehicleDifferential4WData;
}

UENUM(BlueprintType)
namespace EDcxVehicleDifferential4W
{
	enum Type
	{
		LS_4WD = 0		UMETA(DisplayName = "Limited Slip 4WD"),
		LS_FrontWD		UMETA(DisplayName = "Limited Slip Front WD"),
		LS_RearWD		UMETA(DisplayName = "Limited Slip Rear WD"),
		Open_4WD		UMETA(DisplayName = "Open 4WD"),
		Open_FrontWD	UMETA(DisplayName = "Open Front WD"),
		Open_RearWD		UMETA(DisplayName = "Opent Rear WD")
	};
}

USTRUCT(BlueprintType)
struct DCXVEHICLE_API FDcxVehicleDifferentialData4W
{
	GENERATED_USTRUCT_BODY()

public:

	FDcxVehicleDifferentialData4W();

	/**
	 * Type of differential.
	 */
	UPROPERTY(EditAnywhere, Category = "Differential")
	TEnumAsByte<EDcxVehicleDifferential4W::Type> DifferentialType;

	/** 
	 * Ratio of torque split between front and rear (>0.5 means more to front, <0.5 means more to rear).
	 */
	UPROPERTY(EditAnywhere, Category = "Differential")
	float FrontRearSplit;

	/** 
	 * Ratio of torque split between front-left and front-right
	 * (>0.5 means more to front-left, <0.5 means more to front-right).
	 */
	UPROPERTY(EditAnywhere, Category = "Differential")
	float FrontLeftRightSplit;

	/**
	 * Ratio of torque split between rear-left and rear-right
	 * (>0.5 means more to rear-left, <0.5 means more to rear-right).
	 */
	UPROPERTY(EditAnywhere, Category = "Differential")
	float RearLeftRightSplit;

	/**
	 * Maximum allowed ratio of average front wheel rotation speed and rear wheel rotation speeds.
	 * The differential will divert more torque to the slower wheels when the bias is exceeded.
	 * Only applied to Limited Slip 4WD.
	 */
	UPROPERTY(EditAnywhere, Category = "Differential")
	float CenterBias;

	/**
	* Maximum allowed ratio of front-left and front-right wheel rotation speeds.
	* The differential will divert more torque to the slower wheel when the bias is exceeded.
	* Only applied to Limited Slip 4WD and Limited Slip Front WD.
	*/
	UPROPERTY(EditAnywhere, Category = "Differential")
	float FrontBias;

	/**
	* Maximum allowed ratio of rear-left and rear-right wheel rotation speeds.
	* The differential will divert more torque to the slower wheel when the bias is exceeded.
	* Only applied to Limited Slip 4WD and Limited Slip Rear WD.
	*/
	UPROPERTY(EditAnywhere, Category = "Differential")
	float RearBias;

	void Setup(physx::PxVehicleDifferential4WData& PDifferentialData);

};

// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "DcxVehicleTireLoadFilterData.generated.h"

namespace physx
{
	class PxVehicleTireLoadFilterData;
}

USTRUCT(BlueprintType)
struct DCXVEHICLE_API FDcxVehicleTireLoadFilterData
{
	GENERATED_USTRUCT_BODY()

public:

	FDcxVehicleTireLoadFilterData();

	UPROPERTY(EditAnywhere, Category = "TireLoadFilter")
	float MinNormalizedLoad;

	UPROPERTY(EditAnywhere, Category = "TireLoadFilter", AdvancedDisplay)
	float MinFilteredNormalizedLoad;

	UPROPERTY(EditAnywhere, Category = "TireLoadFilter", AdvancedDisplay)
	float MaxNormalizedLoad;

	UPROPERTY(EditAnywhere, Category = "TireLoadFilter", AdvancedDisplay)
	float MaxFilteredNormalizedLoad;

	void Setup(physx::PxVehicleTireLoadFilterData& PTireLoadFilterData);

};

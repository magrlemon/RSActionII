// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "DcxVehicleInputRate.generated.h"

USTRUCT(BlueprintType)
struct DCXVEHICLE_API FDcxVehicleInputRate
{
	GENERATED_USTRUCT_BODY()

public:

	FDcxVehicleInputRate();

	// The rate at which the input value rises.
	UPROPERTY(EditAnywhere, Category="InputRate")
	float RiseRate;

	// The Rate at which the input value falls.
	UPROPERTY(EditAnywhere, Category = "InputRate")
	float FallRate;

	// Change an output value using max rise and fall rates.
	float Interpolate(float CurrentValue, float NewValue, float DeltaTime) const;

};

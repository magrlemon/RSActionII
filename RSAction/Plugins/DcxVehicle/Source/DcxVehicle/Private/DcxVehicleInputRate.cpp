// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "DcxVehicleInputRate.h"

FDcxVehicleInputRate::FDcxVehicleInputRate()
{
	RiseRate = 5.0f;
	FallRate = 5.0f;
}

float FDcxVehicleInputRate::Interpolate(float CurrentValue, float NewValue, float DeltaTime) const
{
	const float DeltaValue = NewValue - CurrentValue;
	const bool bRising = (DeltaValue > 0.0f) == (CurrentValue > 0.0f);
	const float MaxDeltaValue = DeltaTime * (bRising ? RiseRate : FallRate);
	const float ClampedDeltaValue = FMath::Clamp(DeltaValue, -MaxDeltaValue, MaxDeltaValue);
	return CurrentValue + ClampedDeltaValue;
}
// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "DcxMath.generated.h"

USTRUCT()
struct DCXVEHICLE_API FDcxMath
{
	GENERATED_USTRUCT_BODY()

public:

	static inline float CmToM(float Cm)
	{
		return Cm / 1.0e-2;
	}

	static inline float Cm2ToM2(float Cm2)
	{
		return Cm2 / 1.0e-4;
	}

	static inline float CmsToKmh(float Cms)
	{
		return Cms * 36.0e2 / 1.0e5;
	}

	static inline float KmhToCms(float Kmh)
	{
		return Kmh * 1.0e5 / 36.0e2;
	}

	static inline float MToCm(float M)
	{
		return M * 1.0e2;
	}

	static inline float M2ToCm2(float M2)
	{
		return M2 * 1.0e4;
	}

	static inline float RPMToOmega(float RPM)
	{
		return RPM * PI / 30.0f;
	}

	static inline float OmegaToRPM(float Omega)
	{
		return Omega * 30.0f / PI;
	}
};

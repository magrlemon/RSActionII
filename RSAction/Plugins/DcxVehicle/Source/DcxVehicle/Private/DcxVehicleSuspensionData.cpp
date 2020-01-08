// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "DcxVehicleSuspensionData.h"

FDcxVehicleSuspensionData::FDcxVehicleSuspensionData()
{
	NaturalFrequency = 7.0f;
	SpringDamperRatio = 1.0f;

	PxVehicleSuspensionData PSuspension;
	MaxCompression = FDcxMath::MToCm(PSuspension.mMaxCompression);
	MaxDroop =  FDcxMath::MToCm(PSuspension.mMaxDroop);
	CamberAtRest = FMath::RadiansToDegrees(PSuspension.mCamberAtRest);
	CamberAtMaxCompression = FMath::RadiansToDegrees(PSuspension.mCamberAtMaxCompression);
	CamberAtMaxDroop = FMath::RadiansToDegrees(PSuspension.mCamberAtMaxDroop);
}

void FDcxVehicleSuspensionData::Setup(PxVehicleSuspensionData& PSuspensionData)
{
	PSuspensionData.mSpringStrength = FMath::Square(NaturalFrequency) * PSuspensionData.mSprungMass;
	PSuspensionData.mSpringDamperRate = SpringDamperRatio * 2.0f * FMath::Sqrt(PSuspensionData.mSpringStrength * PSuspensionData.mSprungMass);
	PSuspensionData.mMaxCompression = MaxCompression;
	PSuspensionData.mMaxDroop = MaxDroop;
	PSuspensionData.mCamberAtRest = FMath::DegreesToRadians(CamberAtRest);
	PSuspensionData.mCamberAtMaxCompression = FMath::DegreesToRadians(CamberAtMaxCompression);
	PSuspensionData.mCamberAtMaxDroop = FMath::DegreesToRadians(CamberAtMaxDroop);
}

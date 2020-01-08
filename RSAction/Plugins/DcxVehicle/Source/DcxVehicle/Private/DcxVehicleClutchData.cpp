// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "DcxVehicleClutchData.h"

FDcxVehicleClutchData::FDcxVehicleClutchData()
{
	PxVehicleClutchData PClutchData;
	BitePoint = 0.5f;
	Strength = PClutchData.mStrength;
	AccuracyMode = (EDcxVehicleClutchAccuracyMode::Type)((uint8)PClutchData.mAccuracyMode);
	EstimateIterations = PClutchData.mEstimateIterations;
}

void FDcxVehicleClutchData::Setup(PxVehicleClutchData& PClutchData)
{
	PClutchData.mStrength = FDcxMath::M2ToCm2(Strength);
	PClutchData.mAccuracyMode = (PxVehicleClutchAccuracyMode::Enum)((uint8)AccuracyMode);
	PClutchData.mEstimateIterations = EstimateIterations;
}
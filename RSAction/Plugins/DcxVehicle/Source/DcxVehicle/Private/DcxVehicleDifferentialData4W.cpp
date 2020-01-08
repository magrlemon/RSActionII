// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "DcxVehicleDifferentialData4W.h"

FDcxVehicleDifferentialData4W::FDcxVehicleDifferentialData4W()
{
	PxVehicleDifferential4WData PDifferentialData;
	DifferentialType = (EDcxVehicleDifferential4W::Type)((uint32)PDifferentialData.mType);
	FrontRearSplit = PDifferentialData.mFrontRearSplit;
	FrontLeftRightSplit = PDifferentialData.mFrontLeftRightSplit;
	RearLeftRightSplit = PDifferentialData.mRearLeftRightSplit;
	CenterBias = PDifferentialData.mCentreBias;
	FrontBias = PDifferentialData.mFrontBias;
	RearBias = PDifferentialData.mRearBias;
}

void FDcxVehicleDifferentialData4W::Setup(PxVehicleDifferential4WData& PDifferentialData)
{
	PDifferentialData.mType = (PxVehicleDifferential4WData::Enum)((uint32)DifferentialType);
	PDifferentialData.mFrontRearSplit = FrontRearSplit;
	PDifferentialData.mFrontLeftRightSplit = FrontLeftRightSplit;
	PDifferentialData.mRearLeftRightSplit = RearLeftRightSplit;
	PDifferentialData.mCentreBias = CenterBias;
	PDifferentialData.mFrontBias = FrontBias;
	PDifferentialData.mRearBias = RearBias;
}

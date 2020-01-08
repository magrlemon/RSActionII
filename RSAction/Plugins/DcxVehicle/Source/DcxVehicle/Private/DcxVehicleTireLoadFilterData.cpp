// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "DcxVehicleTireLoadFilterData.h"

FDcxVehicleTireLoadFilterData::FDcxVehicleTireLoadFilterData()
{
	PxVehicleTireLoadFilterData PTireLoadFilter;
	MinNormalizedLoad = PTireLoadFilter.mMinNormalisedLoad;
	MinFilteredNormalizedLoad = PTireLoadFilter.mMinFilteredNormalisedLoad;
	MaxNormalizedLoad = PTireLoadFilter.mMaxNormalisedLoad;
	MaxFilteredNormalizedLoad = PTireLoadFilter.mMaxFilteredNormalisedLoad;
}

void FDcxVehicleTireLoadFilterData::Setup(PxVehicleTireLoadFilterData& PTireLoadFilterData)
{
	PTireLoadFilterData.mMinNormalisedLoad = MinNormalizedLoad;
	PTireLoadFilterData.mMinFilteredNormalisedLoad = MinFilteredNormalizedLoad;
	PTireLoadFilterData.mMaxNormalisedLoad = MaxNormalizedLoad;
	PTireLoadFilterData.mMaxFilteredNormalisedLoad = MaxFilteredNormalizedLoad;
}

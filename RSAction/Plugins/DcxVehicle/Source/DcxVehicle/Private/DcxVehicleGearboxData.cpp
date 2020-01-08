// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "DcxVehicleGearboxData.h"

FDcxVehicleGearData::FDcxVehicleGearData()
{
	Ratio = 0;
	DownRatio = 0;
	UpRatio = 0;
}

FDcxVehicleGearData::FDcxVehicleGearData(float GearRatio, float GearDownRatio, float GearUpRatio)
{
	Ratio = GearRatio;
	DownRatio = GearDownRatio;
	UpRatio = GearUpRatio;
}

FDcxVehicleGearboxData::FDcxVehicleGearboxData()
{
	Type = EDcxVehicleGearbox::Automatic;

	PxVehicleGearsData PGearsData;
	PxVehicleAutoBoxData PAutoBoxData;
	SwitchTime = PGearsData.mSwitchTime;
	FinalRatio = PGearsData.mFinalRatio;
	ReverseGearRatio = PGearsData.mRatios[PxVehicleGearsData::eREVERSE];
	Latency = PAutoBoxData.getLatency();
	NeutralGearUpRatio = PAutoBoxData.mUpRatios[PxVehicleGearsData::eNEUTRAL];

	for (uint32 GearNum = PxVehicleGearsData::eFIRST; GearNum < PGearsData.mNbRatios; ++GearNum)
	{
		FDcxVehicleGearData Gear;
		Gear.Ratio = PGearsData.mRatios[GearNum];
		Gear.DownRatio = PAutoBoxData.mDownRatios[GearNum];
		Gear.UpRatio = PAutoBoxData.mUpRatios[GearNum];
		ForwardGears.Add(Gear);
	}
}

void FDcxVehicleGearboxData::Setup(PxVehicleGearsData& PGearsData, PxVehicleAutoBoxData& PAutoBoxData)
{
	PGearsData.mFinalRatio = FinalRatio;
	PGearsData.mSwitchTime = SwitchTime;
	PAutoBoxData.setLatency(Latency);

	PGearsData.mNbRatios = ForwardGears.Num();
	for (int32 GearNum = 0; GearNum < ForwardGears.Num(); ++GearNum)
	{
		PxU32 PGear = GearNum + PxVehicleGearsData::eFIRST;
		PGearsData.mRatios[PGear] = ForwardGears[GearNum].Ratio;
		PAutoBoxData.mDownRatios[PGear] = ForwardGears[GearNum].DownRatio;
		PAutoBoxData.mUpRatios[PGear] = ForwardGears[GearNum].UpRatio;
	}
	PGearsData.mRatios[PxVehicleGearsData::eREVERSE] = ReverseGearRatio;
	PAutoBoxData.mUpRatios[PxVehicleGearsData::eNEUTRAL] = NeutralGearUpRatio;
}

// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "DcxVehicleEngineData.h"

FDcxVehicleEngineData::FDcxVehicleEngineData()
{
	PxVehicleEngineData PEngineData;
	MOI = PEngineData.mMOI;
	MaxRPM = FDcxMath::OmegaToRPM(PEngineData.mMaxOmega);
	DampingRateFullThrottle = PEngineData.mDampingRateFullThrottle;
	DampingRateZeroThrottleClutchEngaged = PEngineData.mDampingRateZeroThrottleClutchEngaged;
	DampingRateZeroThrottleClutchDisengaged = PEngineData.mDampingRateZeroThrottleClutchDisengaged;

	FRichCurve* TorqueCurveData = TorqueCurve.GetRichCurve();
	for (PxU32 KeyIndex = 0; KeyIndex < PEngineData.mTorqueCurve.getNbDataPairs(); KeyIndex++)
	{
		float Time = PEngineData.mTorqueCurve.getX(KeyIndex) * MaxRPM;
		float Value = PEngineData.mTorqueCurve.getY(KeyIndex) * PEngineData.mPeakTorque;
		TorqueCurveData->AddKey(Time, Value);
	}
}

float FDcxVehicleEngineData::GetPeakTorque() const
{
	float PeakTorque = 0.0f;
	TArray<FRichCurveKey> TorqueKeys = TorqueCurve.GetRichCurveConst()->GetCopyOfKeys();
	for (int32 KeyIndex = 0; KeyIndex < TorqueKeys.Num(); KeyIndex++)
	{
		FRichCurveKey& Key = TorqueKeys[KeyIndex];
		PeakTorque = FMath::Max(PeakTorque, Key.Value);
	}
	return PeakTorque;
}

void FDcxVehicleEngineData::Setup(PxVehicleEngineData& PEngineData)
{
	PEngineData.mMOI = FDcxMath::M2ToCm2(MOI);
	PEngineData.mMaxOmega = FDcxMath::RPMToOmega(MaxRPM);
	PEngineData.mDampingRateFullThrottle = FDcxMath::M2ToCm2(DampingRateFullThrottle);
	PEngineData.mDampingRateZeroThrottleClutchEngaged = FDcxMath::M2ToCm2(DampingRateZeroThrottleClutchEngaged);
	PEngineData.mDampingRateZeroThrottleClutchDisengaged = FDcxMath::M2ToCm2(DampingRateZeroThrottleClutchDisengaged);

	float PeakTorque = GetPeakTorque();
	PEngineData.mPeakTorque = FDcxMath::M2ToCm2(PeakTorque);

	PEngineData.mTorqueCurve.clear();
	TArray<FRichCurveKey> TorqueKeys = TorqueCurve.GetRichCurveConst()->GetCopyOfKeys();
	int32 NumTorqueKeys = FMath::Min<int32>(TorqueKeys.Num(), PxVehicleEngineData::eMAX_NB_ENGINE_TORQUE_CURVE_ENTRIES);
	for (int32 KeyIndex = 0; KeyIndex < NumTorqueKeys; ++KeyIndex)
	{
		FRichCurveKey& Key = TorqueKeys[KeyIndex];
		float x = FMath::IsNearlyZero(MaxRPM) ? 0.0f : Key.Time / MaxRPM;
		float y = FMath::IsNearlyZero(PeakTorque) ? 0.0f : Key.Value / PeakTorque;
		PEngineData.mTorqueCurve.addPair(FMath::Clamp(x, 0.0f, 1.0f), FMath::Clamp(y, 0.0f, 1.0f));
	}
}

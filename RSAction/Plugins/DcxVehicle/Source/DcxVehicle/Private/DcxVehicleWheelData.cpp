#include "DcxVehiclePrivatePCH.h"
#include "DcxVehicleWheelData.h"

FDcxVehicleWheelData::FDcxVehicleWheelData()
{
	Radius = 30.0f;
	Width = 10.0f;

	PxVehicleWheelData PWheel;
	Mass = PWheel.mMass;
	DampingRate = PWheel.mDampingRate;
	MaxBrakeTorque = PWheel.mMaxBrakeTorque;
	MaxHandBrakeTorque = PWheel.mMaxHandBrakeTorque;
	MaxSteer = PWheel.mMaxSteer;
	ToeAngle = PWheel.mToeAngle;
}

void FDcxVehicleWheelData::Setup(PxVehicleWheelData& PWheelData)
{
	PWheelData.mRadius = Radius;
	PWheelData.mWidth = Width;
	PWheelData.mMass = Mass;
	PWheelData.mMOI = 0.5f * Mass * FMath::Square(Radius);
	PWheelData.mDampingRate = DampingRate;
	PWheelData.mMaxBrakeTorque = FDcxMath::M2ToCm2(MaxBrakeTorque);
	PWheelData.mMaxHandBrakeTorque = FDcxMath::M2ToCm2(MaxHandBrakeTorque);
	PWheelData.mMaxSteer = FMath::DegreesToRadians(MaxSteer);
	PWheelData.mToeAngle = FMath::DegreesToRadians(ToeAngle);
}

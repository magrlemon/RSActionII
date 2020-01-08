// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "DcxVehicleWheelState.h"

FDcxVehicleWheelState::FDcxVehicleWheelState()
{
	SuspensionLineStart = FVector::ZeroVector;
	SuspensionLineDirection = FVector::ZeroVector;
	SuspensionLineLength = 0.0f;
	bIsInAir = false;
	TireSurfaceMaterial = NULL;
	TireContactPoint = FVector::ZeroVector;
	TireContactNormal = FVector::ZeroVector;
	TireFriction = 0.0f;
	SuspensionJounce = 0.0f;
	SuspensionSpringForce = 0.0f;
	TireLongitudinalDirection = FVector::ZeroVector;
	TireLateralDirection = FVector::ZeroVector;
	LongitudinalSlip = 0.0f;
	LateralSlip = 0.0f;
	SteerAngle = 0.0f;
	LocalPose = FTransform::Identity;
}

void FDcxVehicleWheelState::Setup(PxWheelQueryResult& PQueryResult)
{
	SuspensionLineStart = P2UVector(PQueryResult.suspLineStart);
	SuspensionLineDirection = P2UVector(PQueryResult.suspLineDir);
	SuspensionLineLength = PQueryResult.suspLineLength;
	bIsInAir = PQueryResult.isInAir;
	TireContactPoint = P2UVector(PQueryResult.tireContactPoint);
	TireContactNormal = P2UVector(PQueryResult.tireContactNormal);
	TireFriction = PQueryResult.tireFriction;
	SuspensionJounce = PQueryResult.suspJounce;
	SuspensionSpringForce = PQueryResult.suspSpringForce;
	TireLongitudinalDirection = P2UVector(PQueryResult.tireLongitudinalDir);
	TireLateralDirection = P2UVector(PQueryResult.tireLateralDir);
	LongitudinalSlip = PQueryResult.longitudinalSlip;
	LateralSlip = PQueryResult.lateralSlip;
	SteerAngle = PQueryResult.steerAngle;
	LocalPose = P2UTransform(PQueryResult.localPose);

	if (PQueryResult.tireSurfaceMaterial)
		TireSurfaceMaterial = FPhysxUserData::Get<UPhysicalMaterial>(PQueryResult.tireSurfaceMaterial->userData);
	else
		TireSurfaceMaterial = NULL;
}

// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "PhysicsPublic.h"
#include "DcxVehicleNoDriveComponent.h"

UDcxVehicleNoDriveComponent::UDcxVehicleNoDriveComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

float UDcxVehicleNoDriveComponent::GetSteerAngle(int32 WheelIndex) const
{
	float SteerAngle = 0.0f;
	if (PVehicleWheels)
	{
		FPhysicsCommand::ExecuteRead(UpdatedPrimitive->GetBodyInstance()->ActorHandle, [&](const FPhysicsActorHandle_PhysX& Actor)
		//UpdatedPrimitive->GetBodyInstance()->ExecuteOnPhysicsReadOnly([&]
		{
			PxVehicleNoDrive* PVehicleNoDrive = (PxVehicleNoDrive*)PVehicleWheels;
			SteerAngle = FMath::RadiansToDegrees(PVehicleNoDrive->getSteerAngle(WheelIndex));
		});
	}
	return SteerAngle;
}

void UDcxVehicleNoDriveComponent::SetSteerAngle(int32 WheelIndex, float SteerAngle)
{
	if (PVehicleWheels)
	{
		/*UpdatedPrimitive->GetBodyInstance()->ExecuteOnPhysicsReadWrite([&]
		{
			PxVehicleNoDrive* PVehicleNoDrive = (PxVehicleNoDrive*)PVehicleWheels;
			PVehicleNoDrive->setSteerAngle(WheelIndex, FMath::DegreesToRadians(SteerAngle));
		});*/
	}
}

float UDcxVehicleNoDriveComponent::GetDriveTorque(int32 WheelIndex) const
{
	float DriveTorque = 0.0f;
	if (PVehicleWheels)
	{
		/*UpdatedPrimitive->GetBodyInstance()->ExecuteOnPhysicsReadOnly([&]
		{
			PxVehicleNoDrive* PVehicleNoDrive = (PxVehicleNoDrive*)PVehicleWheels;
			DriveTorque = FDcxMath::Cm2ToM2(PVehicleNoDrive->getDriveTorque(WheelIndex));
		});*/
	}
	return DriveTorque;
}

void UDcxVehicleNoDriveComponent::SetDriveTorque(int32 WheelIndex, float DriveTorque)
{
	if (UpdatedComponent && PVehicleWheels)
	{
		/*UpdatedPrimitive->GetBodyInstance()->ExecuteOnPhysicsReadWrite([&]
		{
			PxVehicleNoDrive* PVehicleNoDrive = (PxVehicleNoDrive*)PVehicleWheels;
			PVehicleNoDrive->setDriveTorque(WheelIndex, FDcxMath::M2ToCm2(DriveTorque));
		});*/
	}
}

float UDcxVehicleNoDriveComponent::GetBrakeTorque(int32 WheelIndex) const
{
	float BrakeTorque = 0.0f;
	if (PVehicleWheels)
	{
		/*UpdatedPrimitive->GetBodyInstance()->ExecuteOnPhysicsReadOnly([&]
		{
			PxVehicleNoDrive* PVehicleNoDrive = (PxVehicleNoDrive*)PVehicleWheels;
			BrakeTorque = FDcxMath::Cm2ToM2(PVehicleNoDrive->getBrakeTorque(WheelIndex));
		});*/
	}
	return BrakeTorque;
}

void UDcxVehicleNoDriveComponent::SetBrakeTorque(int32 WheelIndex, float BrakeTorque)
{
	if (UpdatedComponent && PVehicleWheels)
	{
		/*UpdatedPrimitive->GetBodyInstance()->ExecuteOnPhysicsReadWrite([&]
		{
			PxVehicleNoDrive* PVehicleNoDrive = (PxVehicleNoDrive*)PVehicleWheels;
			PVehicleNoDrive->setBrakeTorque(WheelIndex, FDcxMath::M2ToCm2(BrakeTorque));
		});*/
	}
}

void UDcxVehicleNoDriveComponent::SetupDrive(PxVehicleWheelsSimData* PWheelsSimData)
{
	PxVehicleNoDrive* PVehicleNoDrive = PxVehicleNoDrive::allocate(GetNumWheels());
	
	//ExecuteOnPxRigidDynamicReadWrite(UpdatedPrimitive->GetBodyInstance(), [&](PxRigidDynamic* PVehicleActor)
	FPhysicsCommand::ExecuteWrite(UpdatedPrimitive->GetBodyInstance()->ActorHandle, [&](FPhysicsActorHandle_PhysX& pActor)
	{
		PxRigidDynamic* PVehicleActor = FPhysicsInterface_PhysX::GetPxRigidDynamic_AssumesLocked(pActor);
		PVehicleNoDrive->setup(GPhysXSDK, PVehicleActor, *PWheelsSimData);
		PVehicleNoDrive->setToRestState();
	});

	PVehicleWheels = PVehicleNoDrive;
}

void UDcxVehicleNoDriveComponent::PostDisableWheel(int32 WheelIndex)
{
	PxVehicleNoDrive* PVehicleNoDrive = (PxVehicleNoDrive*)PVehicleWheels;
	PVehicleNoDrive->setDriveTorque(WheelIndex, 0.0f);

	Super::PostDisableWheel(WheelIndex);
}
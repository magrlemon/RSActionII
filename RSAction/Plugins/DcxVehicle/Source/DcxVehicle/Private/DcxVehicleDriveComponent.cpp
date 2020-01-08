// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "DcxVehicleDriveComponent.h"

UDcxVehicleDriveComponent::UDcxVehicleDriveComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoReverse = false;
	WrongDirectionThreshold = 3.6f;
	bAutoBrake = false;
	StopThreshold = 1.8f;
}

FDcxVehicleEngineData UDcxVehicleDriveComponent::GetEngineData() const
{
	return EngineData;
}

void UDcxVehicleDriveComponent::SetEngineData(FDcxVehicleEngineData& NewEngineData)
{
	EngineData = NewEngineData;
}

float UDcxVehicleDriveComponent::GetEngineRotationSpeed() const
{
	float EngineRPM = 0.0f;
	if (PVehicleWheels)
	{
		FPhysicsCommand::ExecuteRead(UpdatedPrimitive->GetBodyInstance()->ActorHandle,[&](const FPhysicsActorHandle_PhysX& Actor)
		//UpdatedPrimitive->GetBodyInstance()->ExecuteOnPhysicsReadOnly([&]
		{
			PxVehicleDrive* PVehicleDrive = (PxVehicleDrive*)PVehicleWheels;
			EngineRPM = PVehicleDrive->mDriveDynData.getEngineRotationSpeed() * 60.0f / (2 * PI);
		});
	}
	return EngineRPM;
}

FDcxVehicleGearboxData UDcxVehicleDriveComponent::GetGearboxData() const
{
	return GearboxData;
}

void UDcxVehicleDriveComponent::SetGearboxData(FDcxVehicleGearboxData& NewGearboxData)
{
	GearboxData = NewGearboxData;
}

EDcxVehicleGearbox::Type UDcxVehicleDriveComponent::GetGearboxType() const
{
	return GearboxData.Type;
}

void UDcxVehicleDriveComponent::SetGearboxType(EDcxVehicleGearbox::Type NewGearboxType)
{
	GearboxData.Type = NewGearboxType;

	if (PVehicleWheels)
	{
		bool bUseAutoGears = NewGearboxType == EDcxVehicleGearbox::Automatic;
		PxVehicleDrive* PVehicleDrive = (PxVehicleDrive*)PVehicleWheels;
		PVehicleDrive->mDriveDynData.setUseAutoGears(bUseAutoGears);
	}
}

int32 UDcxVehicleDriveComponent::GetCurrentGear() const
{
	int32 CurrentGear = 0;
	if (PVehicleWheels)
	{
		PxVehicleDrive* PVehicleDrive = (PxVehicleDrive*)PVehicleWheels;
		CurrentGear = P2UGear(PVehicleDrive->mDriveDynData.getCurrentGear());
	}
	return CurrentGear;
}

int32 UDcxVehicleDriveComponent::GetTargetGear() const
{
	int32 TargetGear = 0;
	if (PVehicleWheels)
	{
		PxVehicleDrive* PVehicleDrive = (PxVehicleDrive*)PVehicleWheels;
		TargetGear = P2UGear(PVehicleDrive->mDriveDynData.getTargetGear());
	}
	return TargetGear;
}

void UDcxVehicleDriveComponent::SetTargetGear(int32 NewGear, bool bImmediate)
{
	if (PVehicleWheels)
	{
		uint32 PNewGear = U2PGear(NewGear);
		PxVehicleDrive* PVehicleDrive = (PxVehicleDrive*)PVehicleWheels;
		if (PVehicleDrive->mDriveDynData.getTargetGear() != PNewGear)
		{
			if (bImmediate)
				PVehicleDrive->mDriveDynData.forceGearChange(PNewGear);
			else
				PVehicleDrive->mDriveDynData.startGearChange(PNewGear);
		}
	}
}

FDcxVehicleClutchData UDcxVehicleDriveComponent::GetClutchData() const
{
	return ClutchData;
}

void UDcxVehicleDriveComponent::SetClutchData(FDcxVehicleClutchData& NewClutchData)
{
	ClutchData = NewClutchData;
}

void UDcxVehicleDriveComponent::CheckAutoReverse(float& Throttle, int32 NumBrakes, float* Brakes)
{
	if (!bAutoReverse || GetGearboxType() != EDcxVehicleGearbox::Automatic)
		return;

	double TotalBrake = 0.0f;
	for (int32 i = 0; i < NumBrakes; ++i)
		TotalBrake += Brakes[i];
	double AverageBrake = TotalBrake / NumBrakes;

	float ForwardSpeed = GetForwardSpeed();
	bool IsAccel = Throttle > 0.0f;
	bool IsBrake = AverageBrake > 0.0f;

	if (FMath::Abs(ForwardSpeed) <= WrongDirectionThreshold)
	{
		if (GetCurrentGear() >= 0 && GetTargetGear() >= 0 && !IsAccel && IsBrake)
			SetTargetGear(-1, true);
		else if (GetCurrentGear() <= 0 && GetTargetGear() <= 0 && IsAccel && !IsBrake)
			SetTargetGear(1, true);
	}

	if (GetCurrentGear() < 0 && !IsAccel && IsBrake)
	{
		Throttle = AverageBrake;
		for (int i = 0; i < NumBrakes; ++i)
			Brakes[i] = 0.0f;
	}
	else if (GetCurrentGear() < 0 && IsAccel && !IsBrake)
	{
		for (int i = 0; i < NumBrakes; ++i)
			Brakes[i] = Throttle;
		Throttle = 0.0f;
	}
}

void UDcxVehicleDriveComponent::CheckAutoBrake(float& Throttle, int32 NumBrakes, float* Brakes)
{
	float ForwardSpeed = GetForwardSpeed();
	if (!bAutoBrake || FMath::Abs(ForwardSpeed) > StopThreshold)
		return;

	bool NoAccel = Throttle == 0.0f;
	bool NoBrake = true;
	for (int32 i = 0; i < NumBrakes; ++i)
	{
		if (Brakes[i] > 0.0f)
		{
			NoBrake = false;
			break;
		}
	}

	if (NoAccel && NoBrake)
	{
		for (int32 i = 0; i < NumBrakes; ++i)
			Brakes[i] = 1.0f;
	}
}

void UDcxVehicleDriveComponent::UpdateClutch(float ClutchInput, PxVehicleClutchData& PClutchData)
{
	ClutchData.Setup(PClutchData);
	if (GetGearboxType() == EDcxVehicleGearbox::Manual)
	{
		if (ClutchInput <= ClutchData.BitePoint)
			PClutchData.mStrength = ClutchData.Strength * (ClutchData.BitePoint - ClutchInput) * 2.0f;
		else
			PClutchData.mStrength = 0.0f;
	}
}

int32 UDcxVehicleDriveComponent::P2UGear(const uint32 PGear) const
{
	if (PGear == PxVehicleGearsData::eREVERSE)
		return -1;

	if (PGear == PxVehicleGearsData::eNEUTRAL)
		return 0;

	return PGear - PxVehicleGearsData::eNEUTRAL;
}

uint32 UDcxVehicleDriveComponent::U2PGear(const int32 UGear) const
{
	if (UGear < 0)
		return PxVehicleGearsData::eREVERSE;

	if (UGear == 0)
		return PxVehicleGearsData::eNEUTRAL;

	return FMath::Min(PxVehicleGearsData::eNEUTRAL + UGear, PxVehicleGearsData::eGEARSRATIO_COUNT - 1);
}

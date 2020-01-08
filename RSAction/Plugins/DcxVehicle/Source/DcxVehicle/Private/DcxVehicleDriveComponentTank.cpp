// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "PhysicsPublic.h"
#include "DcxVehicleDriveComponentTank.h"

UDcxVehicleDriveComponentTank::UDcxVehicleDriveComponentTank(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	WheelConfigurations.SetNum(12);

	DriveModel = EDcxVehicleDriveTankControlModel::Standard;
	ThrustRate.RiseRate = 2.5f;
	ThrustRate.FallRate = 5.0f;
	ThrottleRate.RiseRate = 6.0f;
	ThrottleRate.FallRate = 10.0f;
	BrakeRate.RiseRate = 6.0f;
	BrakeRate.FallRate = 10.0f;

	RawInput = NewObject<UDcxVehicleDriveInputTank>(this, UDcxVehicleDriveInputTank::StaticClass(), FName(TEXT("RawInput")));
	RipeInput = NewObject<UDcxVehicleDriveInputTank>(this, UDcxVehicleDriveInputTank::StaticClass(), FName(TEXT("RipeInput")));
}

void UDcxVehicleDriveComponentTank::SetEngineData(FDcxVehicleEngineData& NewEngineData)
{
	Super::SetEngineData(NewEngineData);

	if (PVehicleWheels)
	{
		PxVehicleEngineData PEngineData;
		NewEngineData.Setup(PEngineData);

		PxVehicleDriveTank* PVehicleDriveTank = (PxVehicleDriveTank*)PVehicleWheels;
		PVehicleDriveTank->mDriveSimData.setEngineData(PEngineData);
	}
}

void UDcxVehicleDriveComponentTank::SetGearboxData(FDcxVehicleGearboxData& NewGearboxData)
{
	Super::SetGearboxData(NewGearboxData);

	if (PVehicleWheels)
	{
		PxVehicleGearsData PGearsData;
		PxVehicleAutoBoxData PAutoBoxData;
		NewGearboxData.Setup(PGearsData, PAutoBoxData);

		PxVehicleDriveTank* PVehicleDriveTank = (PxVehicleDriveTank*)PVehicleWheels;
		PVehicleDriveTank->mDriveSimData.setGearsData(PGearsData);
		PVehicleDriveTank->mDriveSimData.setAutoBoxData(PAutoBoxData);
	}
}

void UDcxVehicleDriveComponentTank::SetClutchData(FDcxVehicleClutchData& NewClutchData)
{
	Super::SetClutchData(NewClutchData);

	if (PVehicleWheels)
	{
		PxVehicleClutchData PClutchData;
		NewClutchData.Setup(PClutchData);

		PxVehicleDriveTank* PVehicleDriveTank = (PxVehicleDriveTank*)PVehicleWheels;
		PVehicleDriveTank->mDriveSimData.setClutchData(PClutchData);
	}
}

EDcxVehicleDriveTankControlModel::Type UDcxVehicleDriveComponentTank::GetDriveModel() const
{
	return DriveModel;
}

void UDcxVehicleDriveComponentTank::SetDriveModel(EDcxVehicleDriveTankControlModel::Type Model)
{
	DriveModel = Model;

	if (PVehicleWheels)
	{
		RawInput->DriveModel = Model;
		RipeInput->DriveModel = Model;

		PxVehicleDriveTank* PVehicleDriveTank = (PxVehicleDriveTank*)PVehicleWheels;
		PVehicleDriveTank->setDriveModel((PxVehicleDriveTankControlModel::Enum)Model);
	}
}

UDcxVehicleDriveInputTank* UDcxVehicleDriveComponentTank::GetInput() const
{
	return RawInput;
}

bool UDcxVehicleDriveComponentTank::CanCreateVehicle() const
{
	if (!Super::CanCreateVehicle())
		return false;

	if (WheelConfigurations.Num() > PX_MAX_NB_WHEELS)
	{
		UE_LOG(DcxVehicleLog, Warning, TEXT("Cannot create vehicle for %s. VehicleTank can have maximum %d wheels."), *GetPathName(), PX_MAX_NB_WHEELS);
		return false;
	}

	return true;
}

void UDcxVehicleDriveComponentTank::SetupDrive(PxVehicleWheelsSimData* PWheelsSimData)
{
	PxVehicleDriveSimData PDriveSimData;
	int32 NumWheels = GetNumWheels();

	PxVehicleEngineData PEngineData;
	EngineData.Setup(PEngineData);
	PDriveSimData.setEngineData(PEngineData);

	PxVehicleGearsData PGearsData;
	PxVehicleAutoBoxData PAutoBoxData;
	GearboxData.Setup(PGearsData, PAutoBoxData);
	PDriveSimData.setGearsData(PGearsData);
	PDriveSimData.setAutoBoxData(PAutoBoxData);

	PxVehicleClutchData PClutchData;
	ClutchData.Setup(PClutchData);
	PDriveSimData.setClutchData(PClutchData);

	PxVehicleDriveTank* PVehicleDriveTank = PxVehicleDriveTank::allocate(NumWheels);
	{
		FPhysicsCommand::ExecuteWrite(UpdatedPrimitive->GetBodyInstance()->ActorHandle, [&](FPhysicsActorHandle_PhysX& pActor)/*(PxRigidDynamic* PVehicleActor)*/
		//ExecuteOnPxRigidDynamicReadWrite(UpdatedPrimitive->GetBodyInstance(), [&](PxRigidDynamic* PVehicleActor)
		{
			PxRigidDynamic* PVehicleActor = FPhysicsInterface_PhysX::GetPxRigidDynamic_AssumesLocked(pActor);
			PVehicleDriveTank->setup(GPhysXSDK, PVehicleActor, *PWheelsSimData, PDriveSimData, NumWheels);
			PVehicleDriveTank->setToRestState();
		});
	}
	

	PVehicleWheels = PVehicleDriveTank;

	SetGearboxType(GearboxData.Type);
	SetDriveModel(DriveModel);
}

void UDcxVehicleDriveComponentTank::UpdateState(float DeltaTime)
{
	APawn* Owner = UpdatedComponent ? Cast<APawn>(UpdatedComponent->GetOwner()) : NULL;
	if (Owner && Owner->IsLocallyControlled())
	{
		float Throttle = RawInput->GetThrottle();
		float Brakes[2];
		Brakes[0] = RawInput->GetLeftBrake();
		Brakes[1] = RawInput->GetRightBrake();
		CheckAutoReverse(Throttle, 2, Brakes);
		CheckAutoBrake(Throttle, 2, Brakes);

		RipeInput->SetThrottle(ThrottleRate.Interpolate(RipeInput->GetThrottle(), Throttle, DeltaTime));
		RipeInput->SetLeftThrust(ThrustRate.Interpolate(RipeInput->GetLeftThrust(), RawInput->GetLeftThrust(), DeltaTime));
		RipeInput->SetRightThrust(ThrustRate.Interpolate(RipeInput->GetRightThrust(), RawInput->GetRightThrust(), DeltaTime));
		RipeInput->SetLeftBrake(BrakeRate.Interpolate(RipeInput->GetLeftBrake(), Brakes[0], DeltaTime));
		RipeInput->SetRightBrake(BrakeRate.Interpolate(RipeInput->GetRightBrake(), Brakes[1], DeltaTime));
		RipeInput->SetGearUp(RawInput->GetGearUp());
		RipeInput->SetGearDown(RawInput->GetGearDown());
	}
}

void UDcxVehicleDriveComponentTank::UpdateSimulation(float DeltaTime)
{
	PxVehicleDriveTank* PVehicleDriveTank = (PxVehicleDriveTank*)PVehicleWheels;

	int32 Mode = (int32)DriveModel;
	PxVehicleDriveTankRawInputData PInputData((PxVehicleDriveTankControlModel::Enum)Mode);
	PInputData.setAnalogAccel(RipeInput->GetThrottle());
	PInputData.setAnalogLeftThrust(RipeInput->GetLeftThrust());
	PInputData.setAnalogRightThrust(RipeInput->GetRightThrust());
	PInputData.setAnalogLeftBrake(RipeInput->GetLeftBrake());
	PInputData.setAnalogRightBrake(RipeInput->GetRightBrake());
	if (!PVehicleDriveTank->mDriveDynData.getUseAutoGears())
	{
		PInputData.setGearUp(RipeInput->GetGearUp());
		PInputData.setGearDown(RipeInput->GetGearDown());
	}

	PxVehicleClutchData PClutchData;
	UpdateClutch(RipeInput->GetClutch(), PClutchData);
	PVehicleDriveTank->mDriveSimData.setClutchData(PClutchData);

	PxVehiclePadSmoothingData PSmoothData = {
		{ ThrottleRate.RiseRate, BrakeRate.RiseRate, BrakeRate.RiseRate, ThrustRate.RiseRate, ThrustRate.RiseRate },
		{ ThrottleRate.FallRate, BrakeRate.FallRate, BrakeRate.FallRate, ThrustRate.FallRate, ThrustRate.FallRate }
	};

	PxVehicleDriveTankSmoothAnalogRawInputsAndSetAnalogInputs(PSmoothData, PInputData, DeltaTime, *PVehicleDriveTank);
}

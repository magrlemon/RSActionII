// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "PhysicsPublic.h"
#include "DcxVehicleDriveComponentNW.h"

UDcxVehicleDriveComponentNW::UDcxVehicleDriveComponentNW(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	WheelConfigurations.SetNum(8);

	SteerRate.RiseRate = 2.5f;
	SteerRate.FallRate = 5.0f;
	ThrottleRate.RiseRate = 6.0f;
	ThrottleRate.FallRate = 10.0f;
	BrakeRate.RiseRate = 6.0f;
	BrakeRate.FallRate = 10.0f;
	HandbrakeRate.RiseRate = 12.0f;
	HandbrakeRate.FallRate = 12.0f;

	FRichCurve* SteerCurveData = SteerCurve.GetRichCurve();
	SteerCurveData->AddKey(0.0f, 1.0f);
	SteerCurveData->AddKey(20.0f, 0.9f);
	SteerCurveData->AddKey(60.0f, 0.8f);
	SteerCurveData->AddKey(120.0f, 0.7f);

	RawInput = NewObject<UDcxVehicleDriveInputNW>(this, UDcxVehicleDriveInputNW::StaticClass(), FName(TEXT("RawInput")));
	RipeInput = NewObject<UDcxVehicleDriveInputNW>(this, UDcxVehicleDriveInputNW::StaticClass(), FName(TEXT("RipeInput")));
}

void UDcxVehicleDriveComponentNW::SetEngineData(FDcxVehicleEngineData& NewEngineData)
{
	Super::SetEngineData(NewEngineData);

	if (PVehicleWheels)
	{
		/*UpdatedPrimitive->GetBodyInstance()->ExecuteOnPhysicsReadWrite([&]
		{
			PxVehicleEngineData PEngineData;
			NewEngineData.Setup(PEngineData);

			PxVehicleDriveNW* PVehicleDriveNW = (PxVehicleDriveNW*)PVehicleWheels;
			PVehicleDriveNW->mDriveSimData.setEngineData(PEngineData);
		});*/
	}
}

void UDcxVehicleDriveComponentNW::SetGearboxData(FDcxVehicleGearboxData& NewGearboxData)
{
	Super::SetGearboxData(NewGearboxData);

	if (PVehicleWheels)
	{
		/*UpdatedPrimitive->GetBodyInstance()->ExecuteOnPhysicsReadWrite([&]
		{
			PxVehicleGearsData PGearsData;
			PxVehicleAutoBoxData PAutoBoxData;
			NewGearboxData.Setup(PGearsData, PAutoBoxData);

			PxVehicleDriveNW* PVehicleDriveNW = (PxVehicleDriveNW*)PVehicleWheels;
			PVehicleDriveNW->mDriveSimData.setGearsData(PGearsData);
			PVehicleDriveNW->mDriveSimData.setAutoBoxData(PAutoBoxData);
		});*/
	}
}

void UDcxVehicleDriveComponentNW::SetClutchData(FDcxVehicleClutchData& NewClutchData)
{
	Super::SetClutchData(NewClutchData);

	if (PVehicleWheels)
	{
		/*UpdatedPrimitive->GetBodyInstance()->ExecuteOnPhysicsReadWrite([&]
		{
			PxVehicleClutchData PClutchData;
			NewClutchData.Setup(PClutchData);

			PxVehicleDriveNW* PVehicleDriveNW = (PxVehicleDriveNW*)PVehicleWheels;
			PVehicleDriveNW->mDriveSimData.setClutchData(PClutchData);
		});*/
	}
}

UDcxVehicleDriveInputNW* UDcxVehicleDriveComponentNW::GetInput() const
{
	return RawInput;
}

bool UDcxVehicleDriveComponentNW::IsDrivenWheel(int32 WheelIndex) const
{
	bool bIsDriven = false;
	if (PVehicleWheels)
	{
		/*UpdatedPrimitive->GetBodyInstance()->ExecuteOnPhysicsReadOnly([&]
		{
			PxVehicleDriveNW* PVehicleDriveNW = (PxVehicleDriveNW*)PVehicleWheels;
			PxVehicleDifferentialNWData PDiffData = PVehicleDriveNW->mDriveSimData.getDiffData();
			bIsDriven = PDiffData.getIsDrivenWheel(WheelIndex);
		});*/
	}
	else
	{
		bIsDriven = WheelConfigurations[WheelIndex].bIsDriven;
	}
	return bIsDriven;
}

void UDcxVehicleDriveComponentNW::SetDrivenWheel(int32 WheelIndex, bool bIsDriven)
{
	WheelConfigurations[WheelIndex].bIsDriven = bIsDriven;

	if (PVehicleWheels)
	{
		/*UpdatedPrimitive->GetBodyInstance()->ExecuteOnPhysicsReadWrite([&]
		{
			PxVehicleDriveNW* PVehicleDriveNW = (PxVehicleDriveNW*)PVehicleWheels;
			PxVehicleDifferentialNWData PDiffData = PVehicleDriveNW->mDriveSimData.getDiffData();
			PDiffData.setDrivenWheel(WheelIndex, bIsDriven);
			PVehicleDriveNW->mDriveSimData.setDiffData(PDiffData);
		});*/
	}
}

bool UDcxVehicleDriveComponentNW::CanCreateVehicle() const
{
	if (!Super::CanCreateVehicle())
		return false;

	if (WheelConfigurations.Num() > PX_MAX_NB_WHEELS)
	{
		UE_LOG(DcxVehicleLog, Warning, TEXT("Cannot create vehicle for %s. VehicleNW can have maximum %d wheels."), *GetPathName(), PX_MAX_NB_WHEELS);
		return false;
	}

	return true;
}

void UDcxVehicleDriveComponentNW::SetupDrive(PxVehicleWheelsSimData* PWheelsSimData)
{
	PxVehicleDriveSimDataNW PDriveSimData;
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

	PxVehicleDifferentialNWData PDiffData;
	for (int32 WheelIndex = 0; WheelIndex < NumWheels; ++WheelIndex)
	{
		FDcxVehicleWheelConfiguration& WheelConfig = WheelConfigurations[WheelIndex];
		PDiffData.setDrivenWheel(WheelIndex, WheelConfig.bIsDriven);
	}
	PDriveSimData.setDiffData(PDiffData);

	PxVehicleDriveNW* PVehicleDriveNW = PxVehicleDriveNW::allocate(NumWheels);
	/*ExecuteOnPxRigidDynamicReadWrite(UpdatedPrimitive->GetBodyInstance(), [&](PxRigidDynamic* PVehicleActor)
	{
		PVehicleDriveNW->setup(GPhysXSDK, PVehicleActor, *PWheelsSimData, PDriveSimData, NumWheels);
		PVehicleDriveNW->setToRestState();
	});*/

	PVehicleWheels = PVehicleDriveNW;

	SetGearboxType(GearboxData.Type);
}

void UDcxVehicleDriveComponentNW::UpdateState(float DeltaTime)
{
	APawn* Owner = UpdatedComponent ? Cast<APawn>(UpdatedComponent->GetOwner()) : NULL;
	if (Owner && Owner->IsLocallyControlled())
	{
		float Throttle = RawInput->GetThrottle();
		float Brake = RawInput->GetBrake();
		CheckAutoReverse(Throttle, 1, &Brake);
		CheckAutoBrake(Throttle, 1, &Brake);

		RipeInput->SetSteer(SteerRate.Interpolate(RipeInput->GetSteer(), RawInput->GetSteer(), DeltaTime));
		RipeInput->SetThrottle(ThrottleRate.Interpolate(RipeInput->GetThrottle(), Throttle, DeltaTime));
		RipeInput->SetBrake(BrakeRate.Interpolate(RipeInput->GetBrake(), Brake, DeltaTime));
		RipeInput->SetClutch(RawInput->GetClutch());
		RipeInput->SetHandbrake(HandbrakeRate.Interpolate(RipeInput->GetHandbrake(), RawInput->GetHandbrake(), DeltaTime));
		RipeInput->SetGearUp(RawInput->GetGearUp());
		RipeInput->SetGearDown(RawInput->GetGearDown());
	}
}

void UDcxVehicleDriveComponentNW::UpdateSimulation(float DeltaTime)
{
	PxVehicleDriveNW* PVehicleDriveNW = (PxVehicleDriveNW*)PVehicleWheels;

	PxVehicleDriveNWRawInputData PInputData;
	PInputData.setAnalogSteer(RipeInput->GetSteer());
	PInputData.setAnalogAccel(RipeInput->GetThrottle());
	PInputData.setAnalogBrake(RipeInput->GetBrake());
	PInputData.setAnalogHandbrake(RipeInput->GetHandbrake());
	if (!PVehicleDriveNW->mDriveDynData.getUseAutoGears())
	{
		PInputData.setGearUp(RipeInput->GetGearUp());
		PInputData.setGearDown(RipeInput->GetGearDown());
	}

	PxVehicleClutchData PClutchData;
	UpdateClutch(RipeInput->GetClutch(), PClutchData);
	PVehicleDriveNW->mDriveSimData.setClutchData(PClutchData);

	PxFixedSizeLookupTable<8> PSteerVsSpeedTable;
	TArray<FRichCurveKey> SteerKeys = SteerCurve.GetRichCurve()->GetCopyOfKeys();
	const int32 NumSamples = FMath::Min(8, SteerKeys.Num());
	for (int32 KeyIndex = 0; KeyIndex < NumSamples; KeyIndex++)
	{
		FRichCurveKey& Key = SteerKeys[KeyIndex];
		PSteerVsSpeedTable.addPair(FDcxMath::KmhToCms(Key.Time), FMath::Clamp(Key.Value, 0.0f, 1.0f));
	}

	PxVehiclePadSmoothingData PSmoothData = {
		{ ThrottleRate.RiseRate, BrakeRate.RiseRate, HandbrakeRate.RiseRate, SteerRate.RiseRate, SteerRate.RiseRate },
		{ ThrottleRate.FallRate, BrakeRate.FallRate, HandbrakeRate.FallRate, SteerRate.FallRate, SteerRate.FallRate }
	};

	PxVehicleDriveNWSmoothAnalogRawInputsAndSetAnalogInputs(PSmoothData, PSteerVsSpeedTable, PInputData, DeltaTime, false, *PVehicleDriveNW);
}

void UDcxVehicleDriveComponentNW::PostDisableWheel(int32 WheelIndex)
{
	PxVehicleDriveNW* PVehicleDriveNW = (PxVehicleDriveNW*)PVehicleWheels;
	PxVehicleDifferentialNWData PDiffData = PVehicleDriveNW->mDriveSimData.getDiffData();
	if (PDiffData.getIsDrivenWheel(WheelIndex))
	{
		WheelConfigurations[WheelIndex].bIsDriven = false;
		PDiffData.setDrivenWheel(WheelIndex, false);
		PVehicleDriveNW->mDriveSimData.setDiffData(PDiffData);
	}

	Super::PostDisableWheel(WheelIndex);
}
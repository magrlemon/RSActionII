// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "PhysicsPublic.h"
#include "DcxVehicleDriveComponent4W.h"

UDcxVehicleDriveComponent4W::UDcxVehicleDriveComponent4W(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	WheelConfigurations.SetNum(4);
	WheelConfigurations[EDcxVehicleWheelOrder4W::FrontLeft].BoneName = FName(TEXT("Front Left"));
	WheelConfigurations[EDcxVehicleWheelOrder4W::FrontRight].BoneName = FName(TEXT("Front Right"));
	WheelConfigurations[EDcxVehicleWheelOrder4W::RearLeft].BoneName = FName(TEXT("Rear Left"));
	WheelConfigurations[EDcxVehicleWheelOrder4W::RearRight].BoneName = FName(TEXT("Rear Right"));

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

	PxVehicleAckermannGeometryData PAckermannGeometryData;
	AckermannGeometryAccuracy = PAckermannGeometryData.mAccuracy;

	RawInput = NewObject<UDcxVehicleDriveInput4W>(this, UDcxVehicleDriveInput4W::StaticClass(), FName(TEXT("RawInput")));
	RipeInput = NewObject<UDcxVehicleDriveInput4W>(this, UDcxVehicleDriveInput4W::StaticClass(), FName(TEXT("RipeInput")));
}

void UDcxVehicleDriveComponent4W::SetEngineData(FDcxVehicleEngineData& NewEngineData)
{
	Super::SetEngineData(NewEngineData);

	if (PVehicleWheels)
	{
		FPhysicsCommand::ExecuteWrite(UpdatedPrimitive->GetBodyInstance()->ActorHandle, [&](const FPhysicsActorHandle_PhysX& Actor)
		//UpdatedPrimitive->GetBodyInstance()->ExecuteOnPhysicsReadWrite([&]
		{
			PxVehicleEngineData PEngineData;
			NewEngineData.Setup(PEngineData);

			PxVehicleDrive4W* PVehicleDrive4W = (PxVehicleDrive4W*)PVehicleWheels;
			PVehicleDrive4W->mDriveSimData.setEngineData(PEngineData);
		});
	}
}

void UDcxVehicleDriveComponent4W::SetGearboxData(FDcxVehicleGearboxData& NewGearboxData)
{
	Super::SetGearboxData(NewGearboxData);

	if (PVehicleWheels)
	{
		/*UpdatedPrimitive->GetBodyInstance()->ExecuteOnPhysicsReadWrite([&]*/
		FPhysicsCommand::ExecuteWrite(UpdatedPrimitive->GetBodyInstance()->ActorHandle, [&](const FPhysicsActorHandle_PhysX& Actor)
		{
			PxVehicleGearsData PGearsData;
			PxVehicleAutoBoxData PAutoBoxData;
			NewGearboxData.Setup(PGearsData, PAutoBoxData);

			PxVehicleDrive4W* PVehicleDrive4W = (PxVehicleDrive4W*)PVehicleWheels;
			PVehicleDrive4W->mDriveSimData.setGearsData(PGearsData);
			PVehicleDrive4W->mDriveSimData.setAutoBoxData(PAutoBoxData);
		});
	}
}

void UDcxVehicleDriveComponent4W::SetClutchData(FDcxVehicleClutchData& NewClutchData)
{
	Super::SetClutchData(NewClutchData);

	if (PVehicleWheels)
	{
		/*UpdatedPrimitive->GetBodyInstance()->ExecuteOnPhysicsReadWrite([&]*/
		FPhysicsCommand::ExecuteWrite(UpdatedPrimitive->GetBodyInstance()->ActorHandle, [&](const FPhysicsActorHandle_PhysX& Actor)
		{
			PxVehicleClutchData PClutchData;
			NewClutchData.Setup(PClutchData);

			PxVehicleDrive4W* PVehicleDrive4W = (PxVehicleDrive4W*)PVehicleWheels;
			PVehicleDrive4W->mDriveSimData.setClutchData(PClutchData);
		});
	}
}

UDcxVehicleDriveInput4W* UDcxVehicleDriveComponent4W::GetInput() const
{
	return RawInput;
}

FDcxVehicleDifferentialData4W UDcxVehicleDriveComponent4W::GetDifferentialData() const
{
	return DifferentialData;
}

void UDcxVehicleDriveComponent4W::SetDifferentialData(FDcxVehicleDifferentialData4W& NewDifferentialData)
{
	DifferentialData = NewDifferentialData;

	if (PVehicleWheels)
	{
		/*UpdatedPrimitive->GetBodyInstance()->ExecuteOnPhysicsReadWrite([&]*/
		FPhysicsCommand::ExecuteWrite(UpdatedPrimitive->GetBodyInstance()->ActorHandle, [&](const FPhysicsActorHandle_PhysX& Actor)
		{
			PxVehicleDifferential4WData PDiffData;
			NewDifferentialData.Setup(PDiffData);

			PxVehicleDrive4W* PVehicleDrive4W = (PxVehicleDrive4W*)PVehicleWheels;
			PVehicleDrive4W->mDriveSimData.setDiffData(PDiffData);
		});
	}
}

bool UDcxVehicleDriveComponent4W::CanCreateVehicle() const
{
	if (!Super::CanCreateVehicle())
		return false;

	if (WheelConfigurations.Num() != 4)
	{
		UE_LOG(DcxVehicleLog, Warning, TEXT("Cannot create vehicle for %s. Vehicle4W must have 4 wheels."), *GetPathName());
		return false;
	}

	return true;
}

void UDcxVehicleDriveComponent4W::SetupDrive(PxVehicleWheelsSimData* PWheelsSimData)
{
	PxVehicleDriveSimData4W PDriveSimData;

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

	PxVehicleDifferential4WData PDiffData;
	DifferentialData.Setup(PDiffData);
	PDriveSimData.setDiffData(PDiffData);

	FVector WheelCenterOffsetFL = P2UVector(PWheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_LEFT));
	FVector WheelCenterOffsetFR = P2UVector(PWheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_RIGHT));
	FVector WheelCenterOffsetRL = P2UVector(PWheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_LEFT));
	FVector WheelCenterOffsetRR = P2UVector(PWheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_RIGHT));

	PxVehicleAckermannGeometryData PAckermannGeometryData;
	PAckermannGeometryData.mAccuracy = AckermannGeometryAccuracy;
	PAckermannGeometryData.mAxleSeparation = FMath::Abs(WheelCenterOffsetFL.X - WheelCenterOffsetRL.X);
	PAckermannGeometryData.mFrontWidth = FMath::Abs(WheelCenterOffsetFR.Y - WheelCenterOffsetFL.Y);
	PAckermannGeometryData.mRearWidth = FMath::Abs(WheelCenterOffsetRR.Y - WheelCenterOffsetRL.Y);
	PDriveSimData.setAckermannGeometryData(PAckermannGeometryData);

	PxVehicleDrive4W* PVehicleDrive4W = PxVehicleDrive4W::allocate(4);
	/*ExecuteOnPxRigidDynamicReadWrite(UpdatedPrimitive->GetBodyInstance(), [&](PxRigidDynamic* PVehicleActor)*/
	FPhysicsCommand::ExecuteWrite(UpdatedPrimitive->GetBodyInstance()->ActorHandle, [&](FPhysicsActorHandle_PhysX& pActor)
	{
		PxRigidDynamic* PVehicleActor = FPhysicsInterface_PhysX::GetPxRigidDynamic_AssumesLocked(pActor);
		PVehicleDrive4W->setup(GPhysXSDK, PVehicleActor, *PWheelsSimData, PDriveSimData, 0);
		PVehicleDrive4W->setToRestState();
	});

	PVehicleWheels = PVehicleDrive4W;

	SetGearboxType(GearboxData.Type);
}

void UDcxVehicleDriveComponent4W::UpdateState(float DeltaTime)
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

void UDcxVehicleDriveComponent4W::UpdateSimulation(float DeltaTime)
{
	PxVehicleDrive4W* PVehicleDrive4W = (PxVehicleDrive4W*)PVehicleWheels;

	PxVehicleDrive4WRawInputData PInputData;
	PInputData.setAnalogSteer(RipeInput->GetSteer());
	PInputData.setAnalogAccel(RipeInput->GetThrottle());
	PInputData.setAnalogBrake(RipeInput->GetBrake());
	PInputData.setAnalogHandbrake(RipeInput->GetHandbrake());
	if (!PVehicleDrive4W->mDriveDynData.getUseAutoGears())
	{
		PInputData.setGearUp(RipeInput->GetGearUp());
		PInputData.setGearDown(RipeInput->GetGearDown());
	}

	PxVehicleClutchData PClutchData;
	UpdateClutch(RipeInput->GetClutch(), PClutchData);
	PVehicleDrive4W->mDriveSimData.setClutchData(PClutchData);

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

	PxVehicleDrive4WSmoothAnalogRawInputsAndSetAnalogInputs(PSmoothData, PSteerVsSpeedTable, PInputData, DeltaTime, false, *PVehicleDrive4W);
}

void UDcxVehicleDriveComponent4W::PostDisableWheel(int32 WheelIndex)
{
	switch (WheelIndex)
	{
	case EDcxVehicleWheelOrder4W::FrontLeft:
		DifferentialData.FrontLeftRightSplit = 0.0f;
		break;

	case EDcxVehicleWheelOrder4W::FrontRight:
		DifferentialData.FrontLeftRightSplit = 1.0f;
		break;

	case EDcxVehicleWheelOrder4W::RearLeft:
		DifferentialData.RearLeftRightSplit = 0.0f;
		break;

	case EDcxVehicleWheelOrder4W::RearRight:
		DifferentialData.RearLeftRightSplit = 1.0f;
		break;
	}

	PxVehicleDifferential4WData PDiffData;
	DifferentialData.Setup(PDiffData);

	PxVehicleDrive4W* PVehicleDrive4W = (PxVehicleDrive4W*)PVehicleWheels;
	PVehicleDrive4W->mDriveSimData.setDiffData(PDiffData);

	Super::PostDisableWheel(WheelIndex);
}
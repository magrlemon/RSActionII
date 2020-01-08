// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "DcxVehicleWheelsComponent.h"
#include "DcxVehicleWheelState.h"
#include "DcxVehicleAnimInstance.h"

FDcxVehicleAnimInstanceProxy::FDcxVehicleAnimInstanceProxy()
	: FAnimInstanceProxy()
{
}

FDcxVehicleAnimInstanceProxy::FDcxVehicleAnimInstanceProxy(UAnimInstance* Instance)
	: FAnimInstanceProxy(Instance)
{
}

void FDcxVehicleAnimInstanceProxy::InitializeAnimData(const UDcxVehicleWheelsComponent* VehicleWheels)
{
	int32 NumWheels = VehicleWheels->GetNumWheels();
	WheelAnimData.Empty(NumWheels);
	WheelAnimData.AddZeroed(NumWheels);

	for (int32 WheelIndex = 0; WheelIndex < NumWheels; ++WheelIndex)
	{
		WheelAnimData[WheelIndex].BoneName = VehicleWheels->GetBoneName(WheelIndex);
		WheelAnimData[WheelIndex].LocalPose = FTransform::Identity;
		WheelAnimData[WheelIndex].IsValid = false;
	}
}

void FDcxVehicleAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	UDcxVehicleAnimInstance* VehicleAnimInstance = CastChecked<UDcxVehicleAnimInstance>(InAnimInstance);
	const UDcxVehicleWheelsComponent* VehicleWheels = VehicleAnimInstance->GetVehicleWheels();
	for (int32 WheelIndex = 0; WheelIndex < WheelAnimData.Num(); ++WheelIndex)
	{
		FDcxVehicleWheelState WheelState;
		if (VehicleWheels->GetWheelState(WheelIndex, WheelState))
		{
			WheelAnimData[WheelIndex].LocalPose = WheelState.LocalPose;
			WheelAnimData[WheelIndex].IsValid = true;
		}
		else
		{
			WheelAnimData[WheelIndex].LocalPose = FTransform::Identity;
			WheelAnimData[WheelIndex].IsValid = false;
		}
	}

	Super::PreUpdate(InAnimInstance, DeltaSeconds);
}

UDcxVehicleAnimInstance::UDcxVehicleAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	VehicleWheels = NULL;
}

const UDcxVehicleWheelsComponent* UDcxVehicleAnimInstance::GetVehicleWheels() const
{
	return VehicleWheels;
}

void UDcxVehicleAnimInstance::SetVehicleWheels(const UDcxVehicleWheelsComponent* InVehicleWheels)
{
	Proxy.InitializeAnimData(InVehicleWheels);
	VehicleWheels = InVehicleWheels;
}

void UDcxVehicleAnimInstance::NativeInitializeAnimation()
{
	if (AActor* Actor = GetOwningActor())
	{
		if (UDcxVehicleWheelsComponent* Component = Actor->FindComponentByClass<UDcxVehicleWheelsComponent>())
		{
			SetVehicleWheels(Component);
		}
	}
}

FAnimInstanceProxy* UDcxVehicleAnimInstance::CreateAnimInstanceProxy()
{
	return &Proxy;
}

void UDcxVehicleAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
}
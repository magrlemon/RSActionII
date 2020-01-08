// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "DcxVehicleAnimInstance.generated.h"

class UDcxVehicleWheelsComponent;

struct FDcxVehicleWheelAnimData
{
	FName BoneName;
	FTransform LocalPose;
	bool IsValid;
};

USTRUCT()
struct DCXVEHICLE_API FDcxVehicleAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

public:

	FDcxVehicleAnimInstanceProxy();
	FDcxVehicleAnimInstanceProxy(UAnimInstance* Instance);

	TArray<FDcxVehicleWheelAnimData> WheelAnimData;

	void InitializeAnimData(const UDcxVehicleWheelsComponent* VehicleWheels);

protected:

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;

};

UCLASS(Transient)
class DCXVEHICLE_API UDcxVehicleAnimInstance : public UAnimInstance
{
	GENERATED_UCLASS_BODY()

public:

	const UDcxVehicleWheelsComponent* GetVehicleWheels() const;
	void SetVehicleWheels(const UDcxVehicleWheelsComponent* InVehicleWheels);

protected:

	FDcxVehicleAnimInstanceProxy Proxy;

	UPROPERTY(Transient)
	const UDcxVehicleWheelsComponent* VehicleWheels;

	virtual void NativeInitializeAnimation() override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;

};

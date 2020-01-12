// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "AnimationRuntime.h"
#include "AnimNode_SkeletalControlBase.h"
#include "DcxAnimNodeWheelSimulator.generated.h"

struct FDcxVehicleAnimInstanceProxy;

struct FDcxVehicleWheelLookup
{
	int32 WheelIndex;
	FBoneReference BoneReference;
};

USTRUCT()
struct DCXVEHICLE_API FDcxAnimNodeWheelSimulator : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

public:

	FDcxAnimNodeWheelSimulator();

	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual bool CanUpdateInWorkerThread() const override;

protected:

	const FDcxVehicleAnimInstanceProxy* Proxy;
	TArray<FDcxVehicleWheelLookup> Lookups;

	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;

};
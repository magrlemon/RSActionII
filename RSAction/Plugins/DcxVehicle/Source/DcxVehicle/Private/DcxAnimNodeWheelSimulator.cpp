// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "Animation/AnimInstanceProxy.h"
#include "PhysicsEngine/BodySetup.h"
#include "DcxVehicle.h"
#include "DcxVehicleWheelState.h"
#include "DcxVehicleAnimInstance.h"
#include "DcxAnimNodeWheelSimulator.h"

FDcxAnimNodeWheelSimulator::FDcxAnimNodeWheelSimulator()
{
	Proxy = NULL;
}

void FDcxAnimNodeWheelSimulator::Initialize(const FAnimationInitializeContext& Context)
{
	Proxy = (FDcxVehicleAnimInstanceProxy*)Context.AnimInstanceProxy;

	Super::Initialize(Context);
}

bool FDcxAnimNodeWheelSimulator::CanUpdateInWorkerThread() const
{
	return false;
}

void  FDcxAnimNodeWheelSimulator::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	if (!Proxy)
		return;

	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
	for (FDcxVehicleWheelLookup& Lookup : Lookups)
	{
		if (!Lookup.BoneReference.IsValid(BoneContainer) || !Proxy->WheelAnimData[Lookup.WheelIndex].IsValid)
			continue;

		FTransform ComponentTM = Output.AnimInstanceProxy->GetComponentTransform();
		FCompactPoseBoneIndex WheelBoneIndex = Lookup.BoneReference.GetCompactPoseIndex(BoneContainer);
		FTransform WheelBoneTM = Proxy->WheelAnimData[Lookup.WheelIndex].LocalPose;
		FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTM, Output.Pose, WheelBoneTM, WheelBoneIndex, BCS_ParentBoneSpace);

		OutBoneTransforms.Add(FBoneTransform(WheelBoneIndex, WheelBoneTM));
	}
}

bool FDcxAnimNodeWheelSimulator::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	if (!Proxy)
		return false;

	for (FDcxVehicleWheelLookup& Lookup : Lookups)
	{
		if (Lookup.BoneReference.IsValid(RequiredBones))
			return true;
	}

	return false;
}

void FDcxAnimNodeWheelSimulator::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	if (!Proxy)
		return;

	int32 NumWheels = Proxy->WheelAnimData.Num();
	Lookups.Empty(NumWheels);
	Lookups.AddZeroed(NumWheels);

	for (int32 Index = 0; Index < NumWheels; ++Index)
	{
		Lookups[Index].WheelIndex = Index;
		Lookups[Index].BoneReference.BoneName = Proxy->WheelAnimData[Index].BoneName;
		Lookups[Index].BoneReference.Initialize(RequiredBones);
	}

	Lookups.Sort([](const FDcxVehicleWheelLookup& Left, const FDcxVehicleWheelLookup& Right)
	{
		return Left.BoneReference.BoneIndex < Right.BoneReference.BoneIndex;
	});
}

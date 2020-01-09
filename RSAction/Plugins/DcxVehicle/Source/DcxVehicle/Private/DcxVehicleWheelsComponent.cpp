// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "Components/SkinnedMeshComponent.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Physics/PhysicsFiltering.h"
#include "DcxVehicleManager.h"
#include "DcxVehicleAnimInstance.h"
#include "DcxVehicleWheelsComponent.h"
#include "PhysicsInterfaceUtilsCore.h"


FDcxVehicleWheelConfiguration::FDcxVehicleWheelConfiguration()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMeshObj(TEXT("/Engine/EngineMeshes/Cylinder"));

	BoneName = NAME_None;
	Offset = FVector::ZeroVector;
	CollisionMesh = StaticMeshObj.Object;
	bUsePresetShape = false;
	Tire = NULL;
}

UDcxVehicleWheelsComponent::UDcxVehicleWheelsComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ThresholdLongitudinalSpeed = 5.0f;
	LowForwardSpeedSubStepCount = 3;
	HighForwardSpeedSubStepCount = 1;

	ChassisMass = 1500.0f;
	InertiaTensorScale = FVector(1.0f, 1.0f, 1.0f);
	DragCoefficient = 0.3f;
	DragArea = 2.0e4;

	PVehicleWheels = NULL;
	VehicleIndex = -1;
}

float UDcxVehicleWheelsComponent::GetChassisMass() const
{
	return ChassisMass;
}

void UDcxVehicleWheelsComponent::SetChassisMass(float Mass)
{
	ChassisMass = Mass;
	bRecreateRequired = true;
}

int32 UDcxVehicleWheelsComponent::GetNumWheels() const
{
	return WheelConfigurations.Num();
}

FName UDcxVehicleWheelsComponent::GetBoneName(int32 WheelIndex) const
{
	return WheelConfigurations[WheelIndex].BoneName;
}

FVector UDcxVehicleWheelsComponent::GetWheelOffset(int32 WheelIndex) const
{
	return WheelConfigurations[WheelIndex].Offset;
}

void UDcxVehicleWheelsComponent::SetWheelOffset(int32 WheelIndex, FVector Offset)
{
	WheelConfigurations[WheelIndex].Offset = Offset;
	bRecreateRequired = true;
}

FDcxVehicleWheelData UDcxVehicleWheelsComponent::GetWheelData(int32 WheelIndex) const
{
	return WheelConfigurations[WheelIndex].WheelData;
}

void UDcxVehicleWheelsComponent::SetWheelData(int32 WheelIndex, FDcxVehicleWheelData& WheelData)
{
	WheelConfigurations[WheelIndex].WheelData = WheelData;
	bRecreateRequired = true;
}

FDcxVehicleSuspensionData UDcxVehicleWheelsComponent::GetSuspensionData(int32 WheelIndex) const
{
	return WheelConfigurations[WheelIndex].SuspensionData;
}

void UDcxVehicleWheelsComponent::SetSuspensionData(int32 WheelIndex, FDcxVehicleSuspensionData& SuspensionData)
{
	WheelConfigurations[WheelIndex].SuspensionData = SuspensionData;
	bRecreateRequired = true;
}

UDcxVehicleTire* UDcxVehicleWheelsComponent::GetTire(int32 WheelIndex) const
{
	return WheelConfigurations[WheelIndex].Tire;
}

void UDcxVehicleWheelsComponent::SetTire(int32 WheelIndex, UDcxVehicleTire* Tire)
{
	WheelConfigurations[WheelIndex].Tire = Tire;

	if (PVehicleWheels)
	{
		PxVehicleTireData PTireData;
		Tire->Setup(PTireData);
		PVehicleWheels->mWheelsSimData.setTireData(WheelIndex, PTireData);
	}
}

float UDcxVehicleWheelsComponent::GetForwardSpeed() const
{
	float ForwardSpeed = 0.0f;
	if (PVehicleWheels)
	{
		/*UpdatedPrimitive->GetBodyInstance()->ExecuteOnPhysicsReadOnly([&]*/
		FPhysicsCommand::ExecuteRead(UpdatedPrimitive->GetBodyInstance()->ActorHandle, [&](const FPhysicsActorHandle_PhysX& Actor)
		{
			ForwardSpeed = FDcxMath::CmsToKmh(PVehicleWheels->computeForwardSpeed());
		});
	}
	return ForwardSpeed;
}

float UDcxVehicleWheelsComponent::GetWheelRotationSpeed(int32 WheelIndex) const
{
	float WheelRotationSpeed = 0.0f;
	if (PVehicleWheels)
	{
		/*UpdatedPrimitive->GetBodyInstance()->ExecuteOnPhysicsReadOnly([&]*/
		FPhysicsCommand::ExecuteRead(UpdatedPrimitive->GetBodyInstance()->ActorHandle, [&](const FPhysicsActorHandle_PhysX& Actor)
		{
			float Radius = WheelConfigurations[WheelIndex].WheelData.Radius;
			float Radians = PVehicleWheels->mWheelsDynData.getWheelRotationSpeed(WheelIndex);
			float Circumference = 2 * PI * Radius;
			WheelRotationSpeed = FDcxMath::CmsToKmh(Circumference * Radians / (2 * PI));
		});
	}
	return WheelRotationSpeed;
}

float UDcxVehicleWheelsComponent::GetWheelRotationAngle(int32 WheelIndex) const
{
	float WheelRotationAngle = 0.0f;
	if (PVehicleWheels)
	{
		/*UpdatedPrimitive->GetBodyInstance()->ExecuteOnPhysicsReadOnly([&]*/
		FPhysicsCommand::ExecuteRead(UpdatedPrimitive->GetBodyInstance()->ActorHandle, [&](const FPhysicsActorHandle_PhysX& Actor)
		{
			WheelRotationAngle = -1.0f * FMath::DegreesToRadians(PVehicleWheels->mWheelsDynData.getWheelRotationAngle(WheelIndex));
		});
	}
	return WheelRotationAngle;
}

bool UDcxVehicleWheelsComponent::GetWheelState(int32 WheelIndex, FDcxVehicleWheelState& WheelState) const
{
	FDcxVehicleManager* VehicleManager = GetVehicleManager();
	if (!VehicleManager)
		return false;

	SCOPED_SCENE_READ_LOCK(VehicleManager->GetScene());

	PxWheelQueryResult* PWheelQueryResults = VehicleManager->GetWheelQueryResults(VehicleIndex);
	if (PWheelQueryResults)
	{
		WheelState.Setup(PWheelQueryResults[WheelIndex]);
		return true;
	}
	else
	{
		return false;
	}
}

void UDcxVehicleWheelsComponent::EnableWheel(int32 WheelIndex)
{
	if (!PVehicleWheels)
		return;
	FPhysicsCommand::ExecuteWrite(UpdatedPrimitive->GetBodyInstance()->ActorHandle,[&](FPhysicsActorHandle_PhysX& Actor)
	//UpdatedPrimitive->GetBodyInstance()->ExecuteOnPhysicsReadWrite([&]
	{
		if (PVehicleWheels->mWheelsSimData.getIsWheelDisabled(WheelIndex))
		{
			PVehicleWheels->mWheelsSimData.enableWheel(WheelIndex);
		}
	});
}

void UDcxVehicleWheelsComponent::DisableWheel(int32 WheelIndex)
{
	if (!PVehicleWheels)
		return;

	//ExecuteOnPxRigidDynamicReadWrite(UpdatedPrimitive->GetBodyInstance(), [&](PxRigidDynamic* PVehicleActor)
	FPhysicsCommand::ExecuteWrite(UpdatedPrimitive->GetBodyInstance()->ActorHandle, [&](FPhysicsActorHandle_PhysX& pActor)
	{
		PxRigidDynamic* PVehicleActor = FPhysicsInterface_PhysX::GetPxRigidDynamic_AssumesLocked(pActor);
		if (!PVehicleWheels->mWheelsSimData.getIsWheelDisabled(WheelIndex))
		{
			PVehicleWheels->mWheelsSimData.setWheelShapeMapping(WheelIndex, -1);
			PVehicleWheels->mWheelsDynData.setWheelRotationSpeed(WheelIndex, 0.0f);
			PVehicleWheels->mWheelsSimData.disableWheel(WheelIndex);
			PostDisableWheel(WheelIndex);
		}
	});
}

bool UDcxVehicleWheelsComponent::IsWheelDisabled(int32 WheelIndex) const
{
	bool IsDisabled = false;
	if (PVehicleWheels)
	{
		/*UpdatedPrimitive->GetBodyInstance()->ExecuteOnPhysicsReadOnly([&]*/
		FPhysicsCommand::ExecuteRead(UpdatedPrimitive->GetBodyInstance()->ActorHandle, [&](const FPhysicsActorHandle_PhysX& Actor)
		{
			IsDisabled = PVehicleWheels->mWheelsSimData.getIsWheelDisabled(WheelIndex);
		});
	}
	return IsDisabled;
}

void UDcxVehicleWheelsComponent::OnCreatePhysicsState()
{
	Super::OnCreatePhysicsState();

	if (!GetWorld()->IsGameWorld())
		return;

	FDcxVehicleManager* VehicleManager = GetVehicleManager();
	if (!VehicleManager)
		return;

	FixupMesh();
	SetupVehicle();

	if (!PVehicleWheels)
		return;

	VehicleIndex = VehicleManager->AddVehicle(this);

	SCOPED_SCENE_WRITE_LOCK(VehicleManager->GetScene());
	PVehicleWheels->getRigidDynamicActor()->wakeUp();

	if (USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(GetMesh()))
	{
		auto Delegate = FOnSkelMeshPhysicsCreated::CreateUObject(this, &UDcxVehicleWheelsComponent::RecreatePhysicsState);
		PhysicsStateChangeHandle = SkeletalMeshComp->RegisterOnPhysicsCreatedDelegate(Delegate);
	}
}

void UDcxVehicleWheelsComponent::OnDestroyPhysicsState()
{
	Super::OnDestroyPhysicsState();

	if (VehicleIndex < 0)
		return;

	FDcxVehicleManager* VehicleManager = GetVehicleManager();
	VehicleManager->RemoveVehicle(VehicleIndex);

	PVehicleWheels->release();
	PVehicleWheels = NULL;
	VehicleIndex = -1;

	if (PhysicsStateChangeHandle.IsValid())
	{
		if (USkeletalMeshComponent* MeshComponent = Cast<USkeletalMeshComponent>(GetMesh()))
			MeshComponent->UnregisterOnPhysicsCreatedDelegate(PhysicsStateChangeHandle);
	}

	if (UpdatedComponent)
		UpdatedComponent->RecreatePhysicsState();
}

bool UDcxVehicleWheelsComponent::ShouldCreatePhysicsState() const
{
	if (!IsRegistered() || IsBeingDestroyed() || !GetWorld()->IsGameWorld())
		return false;

	FDcxVehicleManager* VehicleManager = GetVehicleManager();
	if (!VehicleManager)
		return false;

	return CanCreateVehicle();
}

bool UDcxVehicleWheelsComponent::HasValidPhysicsState() const
{
	return PVehicleWheels != NULL;
}

void UDcxVehicleWheelsComponent::SetUpdatedComponent(USceneComponent* NewUpdatedComponent)
{
	Super::SetUpdatedComponent(NewUpdatedComponent);

	PawnOwner = NewUpdatedComponent ? Cast<APawn>(NewUpdatedComponent->GetOwner()) : nullptr;

	if (USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(NewUpdatedComponent))
	{
		/**
		 * Important!
		 * Waiting Epic Games for proper local space kinematic support.
		 * If not set, the wheels won't sync with the rigid body.
		 */
		SkeletalMeshComp->bLocalSpaceKinematics = true;
	}
}

#if WITH_EDITOR

void UDcxVehicleWheelsComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	bRecreateRequired = true;
}

#endif // WITH_EDITOR

void UDcxVehicleWheelsComponent::PreTickVehicle(float DeltaTime)
{
	if (UpdatedComponent && PVehicleWheels)
	{
		if (APawn* Owner = Cast<APawn>(UpdatedComponent->GetOwner()))
			UpdateState(DeltaTime);
	}

	if (bRecreateRequired)
	{
		RecreatePhysicsState();
		bRecreateRequired = false;
	}
}

void UDcxVehicleWheelsComponent::TickVehicle(float DeltaTime)
{
	if (UpdatedComponent && PVehicleWheels)
	{
		if (APawn* Owner = Cast<APawn>(UpdatedComponent->GetOwner()))
		{
			UpdateSimulation(DeltaTime);
			UpdateDrag(DeltaTime);
		}
	}
}

FDcxVehicleManager* UDcxVehicleWheelsComponent::GetVehicleManager() const
{
	FPhysScene* PhysScene = GetWorld()->GetPhysicsScene();
	return PhysScene ? FDcxVehicleManager::FromScene(PhysScene) : NULL;
}

USkinnedMeshComponent* UDcxVehicleWheelsComponent::GetMesh() const
{
	return Cast<USkinnedMeshComponent>(UpdatedComponent);
}

bool UDcxVehicleWheelsComponent::CanCreateVehicle() const
{
	if (!UpdatedComponent)
	{
		UE_LOG(DcxVehicleLog, Warning, TEXT("Cannot create vehicle for %s. UpdatedComponent is not set."), *GetPathName());
		return false;
	}

	if (!UpdatedPrimitive)
	{
		UE_LOG(DcxVehicleLog, Warning, TEXT("Cannot create vehicle for %s. UpdatedComponent is not a PrimitiveComponent."), *GetPathName());
		return false;
	}

	if (!UpdatedPrimitive->GetBodyInstance())
	{
		UE_LOG(DcxVehicleLog, Warning, TEXT("Cannot create vehicle for %s. UpdatedComponent has not initialized its rigid body actor."), *GetPathName());
		return false;
	}

	if (!UpdatedPrimitive->GetBodyInstance()->IsDynamic())
	{
		UE_LOG(DcxVehicleLog, Warning, TEXT("Cannot create vehicle for %s. UpdatedComponent is not dynamic."), *GetPathName());
		return false;
	}

	for (int32 WheelIndex = 0; WheelIndex < WheelConfigurations.Num(); ++WheelIndex)
	{
		if (WheelConfigurations[WheelIndex].BoneName == NAME_None)
		{
			UE_LOG(DcxVehicleLog, Warning, TEXT("Cannot create vehicle for %s. BoneName for wheel %d is not set."), *GetPathName(), WheelIndex);
			return false;
		}

		if (WheelConfigurations[WheelIndex].Tire == NULL)
		{
			UE_LOG(DcxVehicleLog, Warning, TEXT("Cannot create vehicle for %s. Tire for wheel %d is not set."), *GetPathName(), WheelIndex);
			return false;
		}
	}

	return true;
}

void UDcxVehicleWheelsComponent::SetupVehicle()
{
	SetupWheelShapes();
	SetupChassis();

	int32 NumWheels = WheelConfigurations.Num();
	PxVehicleWheelsSimData* PWheelsSimData = PxVehicleWheelsSimData::allocate(NumWheels);
	SetupWheels(PWheelsSimData);
	SetupDrive(PWheelsSimData);
	PWheelsSimData->free();
}

void UDcxVehicleWheelsComponent::SetupDrive(PxVehicleWheelsSimData* PWheelsSimData)
{
}

void UDcxVehicleWheelsComponent::PostSetupVehicle()
{
}

void UDcxVehicleWheelsComponent::SetupWheels(PxVehicleWheelsSimData* PWheelsSimData)
{
	FPhysicsCommand::ExecuteWrite(UpdatedPrimitive->GetBodyInstance()->ActorHandle, [&](FPhysicsActorHandle_PhysX& pActor)
	//ExecuteOnPxRigidDynamicReadWrite(UpdatedPrimitive->GetBodyInstance(), [&](PxRigidDynamic* PVehicleActor)
	{
		PxRigidDynamic* PVehicleActor = FPhysicsInterface_PhysX::GetPxRigidDynamic_AssumesLocked(pActor);
		PWheelsSimData->setSubStepCount(FDcxMath::MToCm(ThresholdLongitudinalSpeed), LowForwardSpeedSubStepCount, HighForwardSpeedSubStepCount);
		PWheelsSimData->setMinLongSlipDenominator(FDcxMath::MToCm(4.0f));

		int32 NumWheels = WheelConfigurations.Num();
		PxVec3 PWheelCMOffsets[PX_MAX_NB_WHEELS];
		PxF32 PSprungMasses[PX_MAX_NB_WHEELS];

		PxVec3 PChassisCMOffset = U2PVector(GetChassisCMOffset());
		for (int32 WheelIndex = 0; WheelIndex < NumWheels; ++WheelIndex)
			PWheelCMOffsets[WheelIndex] = U2PVector(GetWheelCMOffset(WheelIndex));
		PxVehicleComputeSprungMasses(NumWheels, PWheelCMOffsets, PChassisCMOffset, PVehicleActor->getMass(), 2, PSprungMasses);

		for (int32 WheelIndex = 0; WheelIndex < NumWheels; ++WheelIndex)
		{
			FDcxVehicleWheelConfiguration& WheelConfig = WheelConfigurations[WheelIndex];

			PxVehicleWheelData PWheelData;
			WheelConfig.WheelData.Setup(PWheelData);

			PxVehicleSuspensionData PSuspensionData;
			PSuspensionData.mSprungMass = PSprungMasses[WheelIndex];
			WheelConfig.SuspensionData.Setup(PSuspensionData);

			PxVehicleTireData PTireData;
			if (WheelConfig.Tire)
				WheelConfig.Tire->Setup(PTireData);

			PxVec3 PSuspTravelDirection = PxVec3(0.0f, 0.0f, -1.0f);
			PxVec3 PWheelCenterCMOffset = PWheelCMOffsets[WheelIndex] - PChassisCMOffset;
			PxVec3 PSuspForceAppCMOffset = PWheelCenterCMOffset;
			PxVec3 PTireForceAppCMOffset = PSuspForceAppCMOffset;

			PWheelsSimData->setWheelData(WheelIndex, PWheelData);
			PWheelsSimData->setSuspensionData(WheelIndex, PSuspensionData);
			PWheelsSimData->setTireData(WheelIndex, PTireData);
			PWheelsSimData->setSuspTravelDirection(WheelIndex, PSuspTravelDirection);
			PWheelsSimData->setWheelCentreOffset(WheelIndex, PWheelCenterCMOffset);
			PWheelsSimData->setSuspForceAppPointOffset(WheelIndex, PSuspForceAppCMOffset);
			PWheelsSimData->setTireForceAppPointOffset(WheelIndex, PTireForceAppCMOffset);
		}

		const int32 NumShapes = PVehicleActor->getNbShapes();
		const int32 NumChassisShapes = NumShapes - WheelConfigurations.Num();

		TArray<PxShape*> Shapes;
		Shapes.AddZeroed(NumShapes);
		PVehicleActor->getShapes(Shapes.GetData(), NumShapes);
		for (int32 WheelIndex = 0; WheelIndex < WheelConfigurations.Num(); ++WheelIndex)
		{
			const int32 ShapeIndex = NumChassisShapes + WheelIndex;
			PWheelsSimData->setWheelShapeMapping(WheelIndex, ShapeIndex);
			PWheelsSimData->setSceneQueryFilterData(WheelIndex, Shapes[ShapeIndex]->getQueryFilterData());
		}

		PxVehicleTireLoadFilterData PTireLoadFilter;
		TireLoadFilter.Setup(PTireLoadFilter);
		PWheelsSimData->setTireLoadFilterData(PTireLoadFilter);
	});
}

void UDcxVehicleWheelsComponent::SetupWheelShapes()
{
	PxMaterial* PWheelMaterial = GPhysXSDK->createMaterial(0.0f, 0.0f, 0.0f);

	//ExecuteOnPxRigidDynamicReadWrite(UpdatedPrimitive->GetBodyInstance(), [&](PxRigidDynamic* PVehicleActor)
	FPhysicsCommand::ExecuteWrite(UpdatedPrimitive->GetBodyInstance()->ActorHandle, [&](FPhysicsActorHandle_PhysX& pActor)
	{
		PxRigidDynamic* PVehicleActor = FPhysicsInterface_PhysX::GetPxRigidDynamic_AssumesLocked(pActor);
		for (int32 WheelIndex = 0; WheelIndex < WheelConfigurations.Num(); ++WheelIndex)
		{
			FDcxVehicleWheelConfiguration& WheelConfig = WheelConfigurations[WheelIndex];

			const FVector LocalPose = GetWheelCMOffset(WheelConfig);
			const PxTransform PLocalPose = PxTransform(U2PVector(LocalPose));

			UBodySetup* BodySetup = NULL;
			FVector MeshScale(1.0f, 1.0f, 1.0f);
			PxShape* PWheelShape = NULL;

			if (WheelConfig.bUsePresetShape)
			{
				if (USkinnedMeshComponent* Mesh = GetMesh())
				{
					if (FBodyInstance* BoneInst = Mesh->GetBodyInstance(WheelConfig.BoneName))
						BodySetup = BoneInst->BodySetup.Get();
				}
			}
			else if (WheelConfig.CollisionMesh && WheelConfig.CollisionMesh->BodySetup)
			{
				BodySetup = WheelConfig.CollisionMesh->BodySetup;

				FBoxSphereBounds Bounds = WheelConfig.CollisionMesh->GetBounds();
				MeshScale.X = WheelConfig.WheelData.Radius / Bounds.BoxExtent.X;
				MeshScale.Y = WheelConfig.WheelData.Width / Bounds.BoxExtent.Y;
				MeshScale.Z = WheelConfig.WheelData.Radius / Bounds.BoxExtent.Z;
			}

			if (BodySetup)
			{
				PxMeshScale PMeshScale(U2PVector(UpdatedComponent->RelativeScale3D * MeshScale), PxQuat(PxIdentity));
				if (BodySetup->AggGeom.ConvexElems.Num() == 1)
				{
					PxConvexMesh* PConvexMesh = BodySetup->AggGeom.ConvexElems[0].GetConvexMesh();
					PWheelShape = GPhysXSDK->createShape(PxConvexMeshGeometry(PConvexMesh, PMeshScale), *PWheelMaterial, true);
					PVehicleActor->attachShape(*PWheelShape);
				}
				else if (BodySetup->TriMeshes.Num())
				{
					PxTriangleMesh* PTriMesh = BodySetup->TriMeshes[0];
					PxShapeFlags PShapeFlags = PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eVISUALIZATION;
					PWheelShape = GPhysXSDK->createShape(PxTriangleMeshGeometry(PTriMesh, PMeshScale), *PWheelMaterial, true, PShapeFlags);
					PWheelShape->setLocalPose(PLocalPose);
					PVehicleActor->attachShape(*PWheelShape);
				}
			}

			if (!PWheelShape)
			{
				PxSphereGeometry PSphere(WheelConfig.WheelData.Radius);
				PWheelShape = GPhysXSDK->createShape(PSphere, *PWheelMaterial, true);
			}

			FCollisionResponseContainer CollisionResponse;
			CollisionResponse.SetAllChannels(ECR_Ignore);

			FCollisionFilterData PWheelQueryFilter, PSimQueryFilter;
			uint32 ActorID = UpdatedComponent->GetOwner()->GetUniqueID();
			uint32 ComponentID = UpdatedComponent->GetUniqueID();
			CreateShapeFilterData(ECC_Vehicle, FMaskFilter(0), ActorID, CollisionResponse, ComponentID, 0, PWheelQueryFilter, PSimQueryFilter, false, false, false);

			PWheelShape->setQueryFilterData(U2PFilterData(PWheelQueryFilter));
		}
	});
}

void UDcxVehicleWheelsComponent::SetupChassis()
{
	FPhysicsCommand::ExecuteWrite(UpdatedPrimitive->GetBodyInstance()->ActorHandle, [&](FPhysicsActorHandle_PhysX& pActor)
	//ExecuteOnPxRigidDynamicReadWrite(UpdatedPrimitive->GetBodyInstance(), [&](PxRigidDynamic* PVehicleActor)
	{
		PxRigidDynamic* PVehicleActor = FPhysicsInterface_PhysX::GetPxRigidDynamic_AssumesLocked(pActor);
		const float MassRatio = ChassisMass > 0.0f ? ChassisMass / PVehicleActor->getMass() : 1.0;
		PxVec3 PChassisMOI = PVehicleActor->getMassSpaceInertiaTensor();
		PChassisMOI.x *= InertiaTensorScale.X * MassRatio;
		PChassisMOI.y *= InertiaTensorScale.Y * MassRatio;
		PChassisMOI.z *= InertiaTensorScale.Z * MassRatio;

		PVehicleActor->setMass(ChassisMass);
		PVehicleActor->setMassSpaceInertiaTensor(PChassisMOI);

		const PxVec3 PCMassOffset = U2PVector(GetChassisCMOffset());
		PVehicleActor->setCMassLocalPose(PxTransform(PCMassOffset, PxQuat(PxIdentity)));
	});
}

void UDcxVehicleWheelsComponent::FixupMesh()
{
	USkeletalMeshComponent* Mesh = Cast<USkeletalMeshComponent>(GetMesh());
	if (!Mesh)
		return;

	UPhysicsAsset* PhysicsAsset = Mesh->GetPhysicsAsset();
	if (!PhysicsAsset)
		return;

	for (int32 WheelIndex = 0; WheelIndex < WheelConfigurations.Num(); ++WheelIndex)
	{
		FName BoneName = GetBoneName(WheelIndex);
		if (BoneName == NAME_None)
			continue;

		int32 BodyIndex = PhysicsAsset->FindBodyIndex(BoneName);
		if (BodyIndex < 0)
			continue;

		FBodyInstance* BodyInstance = Mesh->Bodies[BodyIndex];
		BodyInstance->SetResponseToAllChannels(ECR_Ignore);

		UBodySetup* BodySetup = PhysicsAsset->SkeletalBodySetups[BodyIndex];
		if (!BodySetup)
			continue;

		if (BodySetup->PhysicsType == PhysType_Default)
			BodyInstance->SetInstanceSimulatePhysics(false);

		TArray<int32> WheelConstraints;
		PhysicsAsset->BodyFindConstraints(BodyIndex, WheelConstraints);
		for (int32 ConstraintIndex = 0; ConstraintIndex < WheelConstraints.Num(); ++ConstraintIndex)
		{
			FConstraintInstance * ConstraintInstance = Mesh->Constraints[WheelConstraints[ConstraintIndex]];
			ConstraintInstance->TermConstraint();
		}
	}

	Mesh->KinematicBonesUpdateType = EKinematicBonesUpdateToPhysics::SkipSimulatingBones;
}

void UDcxVehicleWheelsComponent::UpdateState(float DeltaTime)
{
}

void UDcxVehicleWheelsComponent::UpdateDrag(float DeltaTime)
{
	if (!PVehicleWheels || !UpdatedPrimitive)
		return;

	float ForwardSpeed = GetForwardSpeed();
	if (FMath::Abs(ForwardSpeed) < 1.0f)
		return;

	FVector ForwardVector = UpdatedComponent->GetForwardVector();
	FVector DragVector = -ForwardVector;
	float SpeedSquared = ForwardSpeed > 0.0f ? ForwardSpeed * ForwardSpeed : -ForwardSpeed * ForwardSpeed;
	float AirDensity = 1.25f / 1.0e6; //kg/cm^3
	float DragMag = 0.5f * AirDensity * SpeedSquared * DragCoefficient * DragArea;
	DragVector *= DragMag;
	FBodyInstance* BodyInstance = UpdatedPrimitive->GetBodyInstance();
	BodyInstance->AddForce(DragVector, false);
}

void UDcxVehicleWheelsComponent::UpdateSimulation(float DeltaTime)
{
}

void UDcxVehicleWheelsComponent::PostDisableWheel(int32 WheelIndex)
{
}

FVector UDcxVehicleWheelsComponent::GetWheelCMOffset(const FDcxVehicleWheelConfiguration& WheelConfig) const
{
	FVector WheelCMOffset = WheelConfig.Offset;
	if (WheelConfig.BoneName != NAME_None)
	{
		USkinnedMeshComponent* Mesh = GetMesh();
		if (Mesh && Mesh->SkeletalMesh)
		{
			const FVector BonePosition = Mesh->SkeletalMesh->GetComposedRefPoseMatrix(WheelConfig.BoneName).GetOrigin() * Mesh->RelativeScale3D;
			const FMatrix BodyMatrix = Mesh->SkeletalMesh->GetComposedRefPoseMatrix(Mesh->GetBodyInstance()->BodySetup->BoneName);
			const FVector LocalBonePosition = BodyMatrix.InverseTransformPosition(BonePosition);
			WheelCMOffset += LocalBonePosition;
		}
	}
	return WheelCMOffset;
}

FVector UDcxVehicleWheelsComponent::GetWheelCMOffset(const int32 WheelIndex) const
{
	return GetWheelCMOffset(WheelConfigurations[WheelIndex]);
}

FVector UDcxVehicleWheelsComponent::GetChassisCMOffset() const
{
	FVector ChassisCMOffset = FVector::ZeroVector;
	if (UpdatedPrimitive)
	{
		if (const FBodyInstance* BodyInstance = UpdatedPrimitive->GetBodyInstance())
		{
			//FPhysicsCommand::ExecuteWrite(UpdatedPrimitive->GetBodyInstance()->ActorHandle, [&](FPhysicsActorHandle_PhysX& pActor)
			FPhysicsCommand::ExecuteRead(UpdatedPrimitive->GetBodyInstance()->ActorHandle, [&](const FPhysicsActorHandle_PhysX& pActor)
			//ExecuteOnPxRigidDynamicReadOnly(BodyInstance, [&](const PxRigidDynamic* PVehicleActor)
			{				
				PxRigidDynamic* PVehicleActor = FPhysicsInterface_PhysX::GetPxRigidDynamic_AssumesLocked(pActor);
				PxTransform PCMassOffset = PVehicleActor->getCMassLocalPose();
				ChassisCMOffset = P2UVector(PCMassOffset.p);
			});
		}
	}
	return ChassisCMOffset;
}

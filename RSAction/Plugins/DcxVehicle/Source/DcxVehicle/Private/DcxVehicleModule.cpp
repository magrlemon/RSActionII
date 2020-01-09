// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "PhysicsPublic.h"
#include "DcxVehicleManager.h"
#include "DcxVehicleWheelsComponent.h"

DEFINE_LOG_CATEGORY(DcxVehicleLog);

class FDcxVehicleModule : public IDcxVehicleModule
{
public:
	
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:

	FDelegateHandle OnUpdatePhysxMaterialHandle;
	FDelegateHandle OnPhysicsAssetChangedHandle;
	FDelegateHandle OnPhysSceneInitHandle;
	FDelegateHandle OnPhysSceneTermHandle;

	void UpdatePhysxMaterial(UPhysicalMaterial* PhysicalMaterial);
	void PhysicsAssetChanged(const UPhysicsAsset* InPhysAsset);
	void PhysSceneInit(FPhysScene* PhysScene/*, EPhysicsSceneType SceneType*/);
	void PhysSceneTerm(FPhysScene* PhysScene/*, EPhysicsSceneType SceneType*/);
};

IMPLEMENT_MODULE(FDcxVehicleModule, DcxVehicle)

void FDcxVehicleModule::StartupModule()
{
	PxInitVehicleSDK(*GPhysXSDK);
	PxVehicleSetBasisVectors(PxVec3(0.0f, 0.0f, 1.0f), PxVec3(1.0f, 0.0f, 0.0f));
	PxVehicleSetUpdateMode(PxVehicleUpdateMode::eVELOCITY_CHANGE);

	OnUpdatePhysxMaterialHandle = FPhysicsDelegates::OnUpdatePhysXMaterial.AddRaw(this, &FDcxVehicleModule::UpdatePhysxMaterial);
	OnPhysicsAssetChangedHandle = FPhysicsDelegates::OnPhysicsAssetChanged.AddRaw(this, &FDcxVehicleModule::PhysicsAssetChanged);
	OnPhysSceneInitHandle = FPhysicsDelegates::OnPhysSceneInit.AddRaw(this, &FDcxVehicleModule::PhysSceneInit);
	OnPhysSceneTermHandle = FPhysicsDelegates::OnPhysSceneTerm.AddRaw(this, &FDcxVehicleModule::PhysSceneTerm);
}

void FDcxVehicleModule::ShutdownModule()
{
	//PxCloseVehicleSDK();
}

void FDcxVehicleModule::UpdatePhysxMaterial(UPhysicalMaterial* PhysicalMaterial)
{
	FDcxVehicleManager::UpdateSurfaceToTireFrictionPairs();
}

void FDcxVehicleModule::PhysicsAssetChanged(const UPhysicsAsset* InPhysAsset)
{
	for (FObjectIterator Iter(UDcxVehicleWheelsComponent::StaticClass()); Iter; ++Iter)
	{
		UDcxVehicleWheelsComponent * VehicleWheelsComp = Cast<UDcxVehicleWheelsComponent>(*Iter);
		if (USkeletalMeshComponent * SkeletalMeshComp = Cast<USkeletalMeshComponent>(VehicleWheelsComp->UpdatedComponent))
		{
			if (SkeletalMeshComp->GetPhysicsAsset() == InPhysAsset)
				VehicleWheelsComp->RecreatePhysicsState();
		}
	}
}

void FDcxVehicleModule::PhysSceneInit(FPhysScene* PhysScene/*, EPhysicsSceneType SceneType*/)
{
	//if (SceneType == PST_Sync)
	{
		new FDcxVehicleManager(PhysScene/*, SceneType*/);
	}
}

void FDcxVehicleModule::PhysSceneTerm(FPhysScene* PhysScene/*, EPhysicsSceneType SceneType*/)
{
	//if (SceneType == PST_Sync)
	{
		FDcxVehicleManager* VehicleManager = FDcxVehicleManager::FromScene(PhysScene);
		if (VehicleManager)
		{
			VehicleManager->DetachFromPhysScene(PhysScene);
			delete VehicleManager;
		}
	}
}

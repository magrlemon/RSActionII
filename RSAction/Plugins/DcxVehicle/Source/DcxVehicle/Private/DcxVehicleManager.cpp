// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "PhysicsPublic.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Physics/PhysicsFiltering.h"
#include "DcxVehicleWheelsComponent.h"
#include "DcxVehicleManager.h"

TMap<FPhysScene*, FDcxVehicleManager*> FDcxVehicleManager::SceneToVehicleManagerMap;
PxVehicleDrivableSurfaceToTireFrictionPairs* FDcxVehicleManager::PSurfaceToTireFrictionPairs = NULL;
bool FDcxVehicleManager::bDirtySurfaceToTireFrictionPairs = false;

static PxQueryHitType::Enum WheelRaycastPreFilter(
	PxFilterData SuspensionData,
	PxFilterData HitData,
	const void* ConstantBlock,
	PxU32 ConstantBlockSize,
	PxHitFlags& FilterFlags)
{
	if (SuspensionData.word0 == HitData.word0)
		return PxQueryHitType::eNONE;

	ECollisionChannel Channel = GetCollisionChannel(SuspensionData.word3);
	if (ECC_TO_BITFIELD(Channel) & HitData.word1)
		return PxQueryHitType::eBLOCK;

	return PxQueryHitType::eNONE;
}

FDcxVehicleManager::FDcxVehicleManager(FPhysScene* PhysScene/*, uint32 SceneType*/)
	: PBatchQuery(NULL)
{
	PScene = PhysScene->GetPxScene();

	OnPhysScenePreTickHandle = PhysScene->OnPhysScenePreTick.AddRaw(this, &FDcxVehicleManager::PreTick);
	OnPhysSceneStepHandle = PhysScene->OnPhysSceneStep.AddRaw(this, &FDcxVehicleManager::Tick);
	
	SceneToVehicleManagerMap.Add(PhysScene, this);
}

FDcxVehicleManager::~FDcxVehicleManager()
{
	while (Vehicles.Num())
	{
		int32 VehicleIndex = Vehicles.Num() - 1;

		PxVehicleWheels* PVehicleWheels = PVehicles[VehicleIndex];
		PVehicleWheels->release();
		
		RemoveVehicle(VehicleIndex);
	}

	if (PBatchQuery)
	{
		PBatchQuery->release();
		PBatchQuery = NULL;
	}
}

FDcxVehicleManager* FDcxVehicleManager::FromScene(FPhysScene* PhysScene)
{
	FDcxVehicleManager** PVehicleManager = SceneToVehicleManagerMap.Find(PhysScene);
	return PVehicleManager ? *PVehicleManager : nullptr;
}

void FDcxVehicleManager::UpdateSurfaceToTireFrictionPairs()
{
	bDirtySurfaceToTireFrictionPairs = true;
}

void FDcxVehicleManager::DetachFromPhysScene(FPhysScene* PhysScene)
{
	PhysScene->OnPhysSceneStep.Remove(OnPhysSceneStepHandle);
	SceneToVehicleManagerMap.Remove(PhysScene);
}

PxScene* FDcxVehicleManager::GetScene() const
{
	return PScene;
}

PxWheelQueryResult* FDcxVehicleManager::GetWheelQueryResults(int32 VehicleIndex)
{
	return PVehicleWheelQueryResults.IsValidIndex(VehicleIndex) ?
		   PVehicleWheelQueryResults[VehicleIndex].wheelQueryResults :
		   NULL;
}

int32 FDcxVehicleManager::AddVehicle(UDcxVehicleWheelsComponent* Vehicle)
{
	check(Vehicle != NULL && Vehicle->PVehicleWheels);

	int32 VehicleIndex = Vehicles.Add(Vehicle);
	PVehicles.Add(Vehicle->PVehicleWheels);

	uint32 NumWheels = Vehicle->PVehicleWheels->mWheelsSimData.getNbWheels();
	PVehicleWheelQueryResults.AddZeroed();
	PVehicleWheelQueryResults[VehicleIndex].nbWheelQueryResults = NumWheels;
	PVehicleWheelQueryResults[VehicleIndex].wheelQueryResults = new PxWheelQueryResult[NumWheels];

	SetupBatchQuery();

	return VehicleIndex;
}

void FDcxVehicleManager::RemoveVehicle(int32 VehicleIndex)
{
	check(VehicleIndex >= 0 && VehicleIndex < Vehicles.Num());

	PRaycastHits.RemoveAt(VehicleIndex);
	PRaycastQueryResults.RemoveAt(VehicleIndex);

	delete[] PVehicleWheelQueryResults[VehicleIndex].wheelQueryResults;
	PVehicleWheelQueryResults.RemoveAt(VehicleIndex);

	PVehicles.RemoveAt(VehicleIndex);
	Vehicles.RemoveAt(VehicleIndex);

	for (int32 Index = VehicleIndex; Index < Vehicles.Num(); ++Index)
		Vehicles[Index]->VehicleIndex = Index;
}

void FDcxVehicleManager::PreTick(FPhysScene* PhysScene, float DeltaTime)
{
	//if (SceneType == PST_Sync)
	{
		for (int32 v = 0; v < Vehicles.Num(); ++v)
		{
			Vehicles[v]->PreTickVehicle(DeltaTime);
		}
	}
}

void FDcxVehicleManager::Tick(FPhysScene* PhysScene, float DeltaTime)
{
	if (/*SceneType != PST_Sync || */Vehicles.Num() == 0)
		return;

	if (bDirtySurfaceToTireFrictionPairs)
	{
		RebuildSurfaceToTireFrictionPairs();
		bDirtySurfaceToTireFrictionPairs = false;
	}

	{
		SCOPED_SCENE_READ_LOCK(PScene);
		PxVehicleSuspensionRaycasts(
			PBatchQuery,
			PVehicles.Num(),
			PVehicles.GetData(),
			PRaycastQueryResults.Num(),
			PRaycastQueryResults.GetData());
	}

	for (int32 v = 0; v < Vehicles.Num(); ++v)
	{
		Vehicles[v]->TickVehicle(DeltaTime);
	}

	{
		SCOPED_SCENE_WRITE_LOCK(PScene);
		PxVec3 Gravity = PScene->getGravity();
		PxVehicleUpdates(
			DeltaTime,
			Gravity,
			*PSurfaceToTireFrictionPairs,
			PVehicles.Num(),
			PVehicles.GetData(),
			PVehicleWheelQueryResults.GetData());
	}
}

void FDcxVehicleManager::RebuildSurfaceToTireFrictionPairs()
{
	const PxU32 MaxNumMaterials = 128;

	PxMaterial* AllPhysicsMaterials[MaxNumMaterials];
	PxVehicleDrivableSurfaceType DrivableSurfaceTypes[MaxNumMaterials];

	uint32 NumMaterials = GPhysXSDK->getMaterials(AllPhysicsMaterials, MaxNumMaterials);
	uint32 NumTires = UDcxVehicleTire::AllTires.Num();

	for (uint32 m = 0; m < NumMaterials; ++m)
		DrivableSurfaceTypes[m].mType = m;

	PSurfaceToTireFrictionPairs = PxVehicleDrivableSurfaceToTireFrictionPairs::allocate(NumTires, NumMaterials);
	PSurfaceToTireFrictionPairs->setup(NumTires, NumMaterials, (const PxMaterial**)AllPhysicsMaterials, DrivableSurfaceTypes);

	for (uint32 m = 0; m < NumMaterials; ++m)
	{
		UPhysicalMaterial* PhysMaterial = FPhysxUserData::Get<UPhysicalMaterial>(AllPhysicsMaterials[m]->userData);
		for (uint32 t = 0; t < NumTires; ++t)
		{
			TWeakObjectPtr<UDcxVehicleTire> Tire = UDcxVehicleTire::AllTires[t];
			float Friction = 1.0f;
			if (Tire != NULL)
				Friction *= Tire->GetFrictionScale(PhysMaterial);
			else if (PhysMaterial != NULL)
				Friction *= PhysMaterial->TireFrictionScale;
			PSurfaceToTireFrictionPairs->setTypePairFriction(m, t, Friction);
		}
	}
}

void FDcxVehicleManager::SetupBatchQuery()
{
	int32 NumWheels = 0;
	for (int32 v = PVehicles.Num() - 1; v >= 0; --v)
		NumWheels += PVehicles[v]->mWheelsSimData.getNbWheels();

	if (NumWheels > PRaycastQueryResults.Num())
	{
		PRaycastQueryResults.AddZeroed(NumWheels - PRaycastQueryResults.Num());
		PRaycastHits.AddZeroed(NumWheels - PRaycastHits.Num());
		check(PRaycastQueryResults.Num() == PRaycastHits.Num());

		if (PBatchQuery)
		{
			PBatchQuery->release();
			PBatchQuery = NULL;
		}

		PxBatchQueryDesc PBatchQueryDesc(NumWheels, 0, 0);
		PBatchQueryDesc.queryMemory.userRaycastResultBuffer = PRaycastQueryResults.GetData();
		PBatchQueryDesc.queryMemory.userRaycastTouchBuffer = PRaycastHits.GetData();
		PBatchQueryDesc.queryMemory.raycastTouchBufferSize = PRaycastHits.Num();
		PBatchQueryDesc.preFilterShader = WheelRaycastPreFilter;

		PBatchQuery = PScene->createBatchQuery(PBatchQueryDesc);
	}
}
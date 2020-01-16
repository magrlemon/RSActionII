// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once
#include "vehicle/PxVehicleUpdate.h"
//class FPhysScene;
class UDcxVehicleWheelsComponent;

namespace physx
{
	class PxBatchQuery;
	class PxScene;
	class PxVehicleWheels;
	class PxVehicleDrivableSurfaceToTireFrictionPairs;
}

class FDcxVehicleManager
{
public:

	FDcxVehicleManager(FPhysScene* PhysScene/*, uint32 SceneType*/);
	~FDcxVehicleManager();

	static FDcxVehicleManager* FromScene(FPhysScene* PhysScene);
	static void UpdateSurfaceToTireFrictionPairs();

	void DetachFromPhysScene(FPhysScene* PhysScene);

	physx::PxScene* GetScene() const;
	physx::PxWheelQueryResult* GetWheelQueryResults(int32 VehicleIndex);

	int32 AddVehicle(UDcxVehicleWheelsComponent* Vehicle);
	void RemoveVehicle(int32 VehicleIndex);
	
	void PreTick(FPhysScene* PhysScene, float DeltaTime);
	void Tick(FPhysScene* PhysScene, float DeltaTime);

private:

	static TMap<FPhysScene*, FDcxVehicleManager*> SceneToVehicleManagerMap;
	static physx::PxVehicleDrivableSurfaceToTireFrictionPairs* PSurfaceToTireFrictionPairs;
	static bool bDirtySurfaceToTireFrictionPairs;

	physx::PxScene* PScene;
	TArray<UDcxVehicleWheelsComponent*> Vehicles;
	TArray<PxVehicleWheels*> PVehicles;
	TArray<PxVehicleWheelQueryResult> PVehicleWheelQueryResults;
	TArray<PxRaycastQueryResult> PRaycastQueryResults;
	TArray<PxRaycastHit> PRaycastHits;
	PxBatchQuery* PBatchQuery;

	FDelegateHandle OnPhysScenePreTickHandle;
	FDelegateHandle OnPhysSceneStepHandle;

	void RebuildSurfaceToTireFrictionPairs();
	void SetupBatchQuery();
};

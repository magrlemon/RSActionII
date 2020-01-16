#include "DcxVehicleAPI.h"
#include "DcxVehicle/Public/DcxVehicleManager.h"
#include "Engine/World.h"

FDcxVehicleManager* dcxMgr = nullptr;

void InitializeDcxVehicle()
{
	dcxMgr = new FDcxVehicleManager(GWorld->GetPhysicsScene());
}
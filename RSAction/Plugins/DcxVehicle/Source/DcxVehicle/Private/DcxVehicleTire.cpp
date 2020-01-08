// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "DcxVehicleManager.h"
#include "DcxVehicleTire.h"

TArray<TWeakObjectPtr<UDcxVehicleTire>> UDcxVehicleTire::AllTires;

FDcxDrivableSurfaceToTireFrictionPair::FDcxDrivableSurfaceToTireFrictionPair()
	: SurfaceMaterial(NULL), FrictionScale(1.0f)
{
}

UDcxVehicleTire::UDcxVehicleTire(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Type = 0;

	PxVehicleTireData PTire;
	LateralStiffnessX = PTire.mLatStiffX;
	LateralStiffnessY = PTire.mLatStiffY;
	LongitudinalStiffnessPerUnitGravity = PTire.mLongitudinalStiffnessPerUnitGravity;
	CamberStiffnessPerUnitGravity = PTire.mCamberStiffnessPerUnitGravity;

	FRichCurve* FrictionVsSlipGraphData = FrictionVsSlipGraph.GetRichCurve();
	FrictionVsSlipGraphData->Reset();
	FrictionVsSlipGraphData->AddKey(PTire.mFrictionVsSlipGraph[0][0], PTire.mFrictionVsSlipGraph[0][1]);
	FrictionVsSlipGraphData->AddKey(PTire.mFrictionVsSlipGraph[1][0], PTire.mFrictionVsSlipGraph[1][1]);
	FrictionVsSlipGraphData->AddKey(PTire.mFrictionVsSlipGraph[2][0], PTire.mFrictionVsSlipGraph[2][1]);
}

UDcxVehicleTire* UDcxVehicleTire::FindTire(uint32 Type)
{
	for (int32 t = 0; t < AllTires.Num(); ++t)
	{
		if (AllTires[t].IsValid() && AllTires[t].Get()->GetType() == Type)
			return AllTires[t].Get();
	}
	return NULL;
}

uint32 UDcxVehicleTire::GetType() const
{
	return Type;
}

float UDcxVehicleTire::GetFrictionScale(TWeakObjectPtr<UPhysicalMaterial> PhysicalMaterial)
{
	float FrictionScale = 1.0f;
	if (PhysicalMaterial != NULL)
	{
		FrictionScale *= PhysicalMaterial->TireFrictionScale;
		for (int32 i = 0; i < DrivableSurfaceToTireFrictionPairs.Num(); ++i)
		{
			if (DrivableSurfaceToTireFrictionPairs[i].SurfaceMaterial == PhysicalMaterial)
			{
				FrictionScale *= DrivableSurfaceToTireFrictionPairs[i].FrictionScale;
				break;
			}
		}
	}
	return FrictionScale;
}

void UDcxVehicleTire::PostInitProperties()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		int32 Index = AllTires.Find(NULL);
		if (Index == INDEX_NONE)
			Index = AllTires.Add(this);
		else
			AllTires[Index] = this;
		Type = Index;

		FDcxVehicleManager::UpdateSurfaceToTireFrictionPairs();
	}
	
	Super::PostInitProperties();
}

void UDcxVehicleTire::BeginDestroy()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		if (AllTires.IsValidIndex(Type) && AllTires[Type] == this)
			AllTires[Type] = NULL;
		
		FDcxVehicleManager::UpdateSurfaceToTireFrictionPairs();
	}

	Super::BeginDestroy();
}

#if WITH_EDITOR

void UDcxVehicleTire::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	FDcxVehicleManager::UpdateSurfaceToTireFrictionPairs();

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

#endif // WITH_EDITOR

void UDcxVehicleTire::Setup(PxVehicleTireData& PTireData)
{
	PTireData.mType = GetType();
	PTireData.mLatStiffX = LateralStiffnessX;
	PTireData.mLatStiffY = LateralStiffnessY;
	PTireData.mLongitudinalStiffnessPerUnitGravity = LongitudinalStiffnessPerUnitGravity;
	PTireData.mCamberStiffnessPerUnitGravity = CamberStiffnessPerUnitGravity;
	FRichCurve* FrictionVsSlipGraphData = FrictionVsSlipGraph.GetRichCurve();
	if (FrictionVsSlipGraphData->GetNumKeys() == 3)
	{
		for (int32 KeyIndex = 0; KeyIndex < 3; ++KeyIndex)
		{
			FRichCurveKey& Key = FrictionVsSlipGraphData->Keys[KeyIndex];
			PTireData.mFrictionVsSlipGraph[KeyIndex][0] = Key.Time;
			PTireData.mFrictionVsSlipGraph[KeyIndex][1] = Key.Value;
		}
	}
}

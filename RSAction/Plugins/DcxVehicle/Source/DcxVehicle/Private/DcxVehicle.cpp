// Copyright 2016 Dotex Games. All Rights Reserved.

#include "DcxVehiclePrivatePCH.h"
#include "DcxVehicleWheelsComponent.h"
#include "DcxVehicle.h"

ADcxVehicle::ADcxVehicle(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VehicleMeshComponent"));
	Mesh->SetCollisionProfileName(UCollisionProfile::Vehicle_ProfileName);
	Mesh->BodyInstance.bSimulatePhysics = true;
	Mesh->BodyInstance.bNotifyRigidBodyCollision = true;
	Mesh->BodyInstance.bUseCCD = true;
	Mesh->bBlendPhysics = true;
	Mesh->SetGenerateOverlapEvents(true);
	Mesh->SetCanEverAffectNavigation(false);
	RootComponent = Mesh;
}

void ADcxVehicle::DcxVehicleSetBasisVectors(const FVector& Up, const FVector& Forward)
{
	PxVehicleSetBasisVectors(U2PVector(Up), U2PVector(Forward));
}

USkeletalMeshComponent* ADcxVehicle::GetMesh() const
{
	return Mesh;
}

UDcxVehicleWheelsComponent* ADcxVehicle::GetVehicleWheels() const
{
	return VehicleWheels;
}

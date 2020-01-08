// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "GameFramework/Pawn.h"
#include "DcxVehicle.generated.h"

class UVehicleWheelsComponent;
class USkeletalMeshComponent;

UCLASS(Abstract, BlueprintType, ClassGroup = "Dcx|Vehicle")
class DCXVEHICLE_API ADcxVehicle : public APawn
{
	GENERATED_UCLASS_BODY()

protected:

	/**
	 * The skeletal mesh associated with this vehicle.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Category = "Vehicle")
	USkeletalMeshComponent* Mesh;

	/**
	 *  The vehicle simulation component.
	 */
	UPROPERTY(BlueprintReadonly, VisibleDefaultsOnly, Category = "Vehicle")
	UDcxVehicleWheelsComponent* VehicleWheels;

public:

	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle")
	static void DcxVehicleSetBasisVectors(const FVector& Up, const FVector& Forward);

	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle")
	virtual USkeletalMeshComponent* GetMesh() const;

	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle")
	virtual UDcxVehicleWheelsComponent* GetVehicleWheels() const;

};

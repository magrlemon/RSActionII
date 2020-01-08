// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "DcxVehicleTire.generated.h"

namespace physx
{
	class PxVehicleTireData;
}

USTRUCT()
struct DCXVEHICLE_API FDcxDrivableSurfaceToTireFrictionPair
{
	GENERATED_USTRUCT_BODY()

public:

	FDcxDrivableSurfaceToTireFrictionPair();

	UPROPERTY(EditAnywhere, Category = "DrivableSurfaceToTireFrictionPair")
	UPhysicalMaterial* SurfaceMaterial;

	UPROPERTY(EditAnywhere, Category = "DrivableSurfaceToTireFrictionPair")
	float FrictionScale;
};

UCLASS(ClassGroup = "Dcx|Vehicle")
class DCXVEHICLE_API UDcxVehicleTire : public UDataAsset
{
	GENERATED_UCLASS_BODY()

private:

	uint32 Type;

public:

	static TArray<TWeakObjectPtr<UDcxVehicleTire>> AllTires;

	static UDcxVehicleTire* FindTire(uint32 Type);

	/**
	 * Tire lateral stiffness is a graph of tire load that has linear behavior near zero load and flattens at large loads.
	 * Describes the minimum normalized load (Load / RestLoad) that gives a flat lateral stiffness response to load.
	 * Specified in per radian.
	 */
	UPROPERTY(EditAnywhere, Category = "Tire")
	float LateralStiffnessX;

	/**
	 * Tire lateral stiffness is a graph of tire load that has linear behavior near zero load and flattens at large loads.
	 * Describes the maximum possible value of LateralStiffness / RestLoad that occurs when (Load / RestLoad) >= LateralStiffnessX.
	 * Specified in per radian.
	 */
	UPROPERTY(EditAnywhere, Category = "Tire")
	float LateralStiffnessY;

	/*
	 * Tire Longitudinal stiffness per unit gravitational acceleration.
	 * Specified in kilograms per radian.
	 */
	UPROPERTY(EditAnywhere, Category = "Tire")
	float LongitudinalStiffnessPerUnitGravity;

	/**
	 * Tire camber stiffness per unity gravitational acceleration.
	 * Specified in kilograms per radian.
	 */
	UPROPERTY(EditAnywhere, Category = "Tire")
	float CamberStiffnessPerUnitGravity;

	/**
	 * Graph of friction vs longitudinal slip with 3 points.
	 * This curve must have exactly 3 points.
	 * Otherwise default values are assumed.
	 */
	UPROPERTY(EditAnywhere, Category = "Tire")
	FRuntimeFloatCurve FrictionVsSlipGraph;

	/**
	 * Tire friction multiplier.
	 */
	UPROPERTY(EditAnywhere, Category = "Friction")
	TArray<FDcxDrivableSurfaceToTireFrictionPair> DrivableSurfaceToTireFrictionPairs;

	// Gets the type of tire to pass to PhysX.
	uint32 GetType() const;

	virtual float GetFrictionScale(TWeakObjectPtr<UPhysicalMaterial> PhysicalMaterial);

	virtual void PostInitProperties() override;

	virtual void BeginDestroy() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	void Setup(physx::PxVehicleTireData& PTireData);

};

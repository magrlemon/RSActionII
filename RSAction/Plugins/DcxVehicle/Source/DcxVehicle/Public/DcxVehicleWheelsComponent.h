// Copyright 2016 Dotex Games. All Rights Reserved.

#pragma once

#include "GameFramework/PawnMovementComponent.h"
#include "DcxVehicleWheelData.h"
#include "DcxVehicleSuspensionData.h"
#include "DcxVehicleTire.h"
#include "DcxVehicleTireLoadFilterData.h"
#include "DcxVehicleWheelState.h"
#include "DcxVehicleWheelsComponent.generated.h"

class FDcxVehicleManager;

namespace physx
{
	class PxVehicleWheels;
	class PxVehicleWheelsSimData;
}

struct FDcxVehicleWheelPoseData
{
	FVector LocationOffset;
	FRotator RotationOffset;
};

USTRUCT(BlueprintType)
struct DCXVEHICLE_API FDcxVehicleWheelConfiguration
{
	GENERATED_USTRUCT_BODY()

public:

	FDcxVehicleWheelConfiguration();

	/**
	 * Bone name on skeletal mesh to create wheel at.
	 */
	UPROPERTY(EditAnywhere, Category = "WheelConfiguration")
	FName BoneName;

	/**
	 * If BoneName is specified, offset the wheel from the bone's location.
	 * Otherwise this offsets the wheel from the vehicle's origin.
	 */
	UPROPERTY(EditAnywhere, Category = "WheelConfiguration")
	FVector Offset;

	/**
	 * Static mesh that will be used to create wheel shape.
	 * If empty, a sphere will be added as wheel shape.
	 */
	UPROPERTY(EditAnywhere, Category = "WheelConfiguration", Meta = (EditCondition = "!bUsePresetShape"))
	UStaticMesh* CollisionMesh;

	/**
	 * If set, a shape won't be created, but mapped from chassis mesh.
	 */
	UPROPERTY(EditAnywhere, Category = "WheelConfiguration")
	bool bUsePresetShape;

	/**
	 * Wheel data for this wheel.
	 */
	UPROPERTY(EditAnywhere, Category = "WheelConfiguration")
	FDcxVehicleWheelData WheelData;

	/**
	 * Suspension data for this wheel.
	 */
	UPROPERTY(EditAnywhere, Category = "WheelConfiguration")
	FDcxVehicleSuspensionData SuspensionData;

	/**
	 * Tire to use for this wheel.
	 */
	UPROPERTY(EditAnywhere, Category = "WheelConfiguration")
	UDcxVehicleTire* Tire;

	/**
	 * Specifies whether wheel has been configured as a driven or non-driven wheel.
	 * This property is ignored except DcxVehicleDriveComponentNW.
	 */
	UPROPERTY(EditAnywhere, Category = "WheelConfiguration")
	bool bIsDriven;
};

UCLASS(Abstract, HideCategories = (PlanarMovement, "Components|Movement|Planar", Activation, "Components|Activation"))
class DCXVEHICLE_API UDcxVehicleWheelsComponent : public UPawnMovementComponent
{
	GENERATED_UCLASS_BODY()

protected:

	/**
	 * Speed used to categorize vehicle speed as low speed or high speed.
	 */
	UPROPERTY(EditAnywhere, Category = "Sub-Stepping")
	float ThresholdLongitudinalSpeed;

	/**
	 * The number of sub-steps performed in vehicle updates that have
	 * longitudinal speed lower than ThresholdLongitudinalSpeed.
	 */
	UPROPERTY(EditAnywhere, Category = "Sub-Stepping")
	int32 LowForwardSpeedSubStepCount;

	/**
	 * The number of sub-steps performed in vehicle updates that
	 * have longitudinal speed greater than ThresholdLongitudinalSpeed.
	 */
	UPROPERTY(EditAnywhere, Category = "Sub-Stepping")
	int32 HighForwardSpeedSubStepCount;

	/**
	 * The mass of the chassis.
	 * Assumes that the suspensions equally share the load of the chassis mass and will have
	 * a particular natural frequency and damping ratio that is typical of a standard car.
	 * Specified in kilograms (kg).
	 */
	UPROPERTY(EditAnywhere, Category = "Chassis")
	float ChassisMass;

	/**
	 * Scales the moment of inertia of vehicle rigid body actor.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Chassis")
	FVector InertiaTensorScale;

	/**
	 * Drag resistance of the vehicle (Cd).
	 * A lower drag coefficient indicates the vehicle has less aerodynamic drag.
	 */
	UPROPERTY(EditAnywhere, Category = "Aerodynamics")
	float DragCoefficient;

	/**
	 * The reference area which is subject to drag resistance.
	 */
	UPROPERTY(EditAnywhere, Category = "Aerodynamics")
	float DragArea;

	/**
	 * Description of wheels to create.
	 * A Vehicle4W must have exactly 4 driven wheels.
	 * The wheel setups for Vehicle4W must have the order: FL, FR, RL, RR.
	 * A VehicleNW can have up to 20 driven wheels.
	 */
	UPROPERTY(EditAnywhere, Category = "Suspension")
	TArray<FDcxVehicleWheelConfiguration> WheelConfigurations;

	/**
	 * Tire load variation can be strongly dependent on the time-step so
	 * it is a good idea to filter it to give less jerky handling behavior.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Suspension")
	FDcxVehicleTireLoadFilterData TireLoadFilter;

public:

	/**
	 * Gets the mass of the chassis.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleWheelsComponent")
	float GetChassisMass() const;

	/**
	 * Sets the mass of the chassis.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleWheelsComponent")
	void SetChassisMass(float Mass);

	/**
	 * Gets the number of wheels.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleWheelsComponent")
	int32 GetNumWheels() const;

	/**
	 * Gets the bone name of the specified wheel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleWheelsComponent")
	FName GetBoneName(int32 WheelIndex) const;

	/**
	 * Gets the offset of the specified wheel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleWheelsComponent")
	FVector GetWheelOffset(int32 WheelIndex) const;

	/**
	 * Sets the offset of the specified wheel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleWheelsComponent")
	void SetWheelOffset(int32 WheelIndex, FVector Offset);

	/**
	 * Gets the wheel data of the specified wheel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleWheelsComponent")
	FDcxVehicleWheelData GetWheelData(int32 WheelIndex) const;

	/**
	 * Sets the wheel data of the specified wheel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleWheelsComponent")
	void SetWheelData(int32 WheelIndex, FDcxVehicleWheelData& WheelData);


	/**
	* Gets the suspension data of the specified wheel.
	*/
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleWheelsComponent")
	FDcxVehicleSuspensionData GetSuspensionData(int32 WheelIndex) const;

	/**
	 * Sets the suspension data of the specified wheel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleWheelsComponent")
	void SetSuspensionData(int32 WheelIndex, FDcxVehicleSuspensionData& SuspensionData);

	/**
	 * Gets the tire of the specified wheel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleWheelsComponent")
	UDcxVehicleTire* GetTire(int32 WheelIndex) const;

	/**
	 * Sets the tire of the specified wheel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleWheelsComponent")
	void SetTire(int32 WheelId, UDcxVehicleTire* Tire);

	/**
	 * Gets how fast the vehicle is moving forward (km/h).
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleWheelsComponent")
	float GetForwardSpeed() const;

	/**
	 * Gets the rotation speed about the rolling axis of the specified wheel.
	 * Specified in kilometers per hour (km/h).
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleWheelsComponent")
	float GetWheelRotationSpeed(int32 WheelIndex) const;

	/**
	 * Gets the rotation angle about the rolling axis for the specified wheel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleWheelsComponent")
	float GetWheelRotationAngle(int32 WheelIndex) const;

	/**
	 * Gets the wheel state of the specified wheel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleWheelsComponent")
	bool GetWheelState(int32 WheelIndex, FDcxVehicleWheelState& WheelState) const;

	/**
	 * Enable a wheel so that suspension forces and tire forces are applied to the rigid body.
	 * All wheels are enabled by default and remain enabled until they are disabled.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleWheelsComponent")
	void EnableWheel(int32 WheelIndex);

	/**
	 * Disable a wheel so that zero suspension forces and zero tire forces are applied to the rigid body from this wheel.
	 * If the vehicle has a differential, then the differential needs to be configured so that no drive torque is delivered to the disabled wheel.
	 * If the vehicle is of type DcxVehicleNoDrive then zero drive torque must be applied to the disabled wheel.
	 * For tanks, any drive torque that could be delivered to the wheel through the tank differential will be re-directed to the remaining enabled wheels.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleWheelsComponent")
	void DisableWheel(int32 WheelIndex);

	/**
	 * Test if a wheel has been disabled.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dcx|Vehicle|VehicleWheelsComponent")
	bool IsWheelDisabled(int32 WheelIndex) const;

public:

	virtual void OnCreatePhysicsState() override;
	virtual void OnDestroyPhysicsState() override;
	virtual bool ShouldCreatePhysicsState() const override;
	virtual bool HasValidPhysicsState() const override;
	virtual void SetUpdatedComponent(USceneComponent* NewUpdatedComponent) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void PreTickVehicle(float DeltaTime);
	virtual void TickVehicle(float DeltaTime);

public:

	physx::PxVehicleWheels* PVehicleWheels;
	int32 VehicleIndex;

protected:

	FDelegateHandle PhysicsStateChangeHandle;
	bool bRecreateRequired;

	FDcxVehicleManager* GetVehicleManager() const;
	USkinnedMeshComponent* GetMesh() const;

	virtual bool CanCreateVehicle() const;
	virtual void SetupVehicle();
	virtual void SetupDrive(physx::PxVehicleWheelsSimData* PWheelsSimData);
	virtual void PostSetupVehicle();
	virtual void SetupWheels(physx::PxVehicleWheelsSimData* PWheelsSimData);
	virtual void SetupWheelShapes();
	virtual void SetupChassis();
	virtual void FixupMesh();
	virtual void UpdateState(float DeltaTime);
	virtual void UpdateDrag(float DeltaTime);
	virtual void UpdateSimulation(float DeltaTime);
	virtual void PostDisableWheel(int32 WheelIndex);

	FVector GetWheelCMOffset(const int32 WheelIndex) const;
	FVector GetWheelCMOffset(const FDcxVehicleWheelConfiguration& WheelConfig) const;
	FVector GetChassisCMOffset() const;
};
#pragma once

#include "Vehicle.generated.h"
class AActor;

UINTERFACE(BlueprintType)
class UVehicle : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class IVehicle
{
	GENERATED_IINTERFACE_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MyCategory")
	void MoveForwordImpl(float forward, float right);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MyCategory")
	void MoveRightImpl(float forward, float right);
	/*UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MyCategory")
	AActor* GetVehicleActor();*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MyCategory")
	bool DetectInArea(AActor* enterActor);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MyCategory")
	void AimAzimuth(float value);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MyCategory")
	void AimElevation(float value);

public:
	
	//FVector reletiveLoginPos = FVector::ZeroVector;
};
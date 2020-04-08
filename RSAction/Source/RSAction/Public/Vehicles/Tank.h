// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Vehicle.h"
#include "GameFramework/Pawn.h"
#include "Tank.generated.h"

//class AVehicleImpactEffect;
//class UCamouflageComponent;
//class UVehicleDustType;
class UTankCameraMovementComponent;
class UTankMainWeaponComponent;
class UTankMovementComponent;
//class UTankSpottingComponent;

class UAudioComponent;
class UCameraShake;
class UParticleSystemComponent;
class USoundCue;
class UTrackComponent;
class UInstancedStaticMeshComponent;
class UCameraComponent;

UCLASS()
class ATank : public APawn, public IVehicle
{
	GENERATED_BODY()

private:
	UPROPERTY(Transient)
		TArray<UParticleSystemComponent *> DustParticleComponents;

	bool bHighlighting = false;
	bool bSkidding = false;
	bool bOnAir = false;
	bool bFullThrottleSfxPlayed = false;
	bool bTrackRollingSfxPlaying = false;
	float SkidStartTime = 0;
	int RemainingHitpoint = 100;
	int ZoomIndex = 0;
	bool bFirstCameraView = false;

protected:
	/** The point where AI should aim at */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = AI)
		USceneComponent * AITarget;

	UPROPERTY(VisibleAnywhere, Category = Components)
		UAudioComponent* EngineAudioComponent;
	UPROPERTY(VisibleAnywhere, Category = Components)
		UAudioComponent* SkidAudioComponent;
	UPROPERTY(VisibleAnywhere, Category = Components)
		UAudioComponent * TurretRotateAudioComponent;
	UPROPERTY(VisibleAnywhere, Category = Components)
		UAudioComponent * TrackRollingAudioComponent;
	UPROPERTY(VisibleAnywhere, Category = Components)
		UAudioComponent * FullThrottleRotateAudioComponent;

	UPROPERTY(VisibleAnywhere, Category = Components)
		USpringArmComponent* FirstSpringArm;
	UPROPERTY(VisibleAnywhere, Category = Components)
		USpringArmComponent* ThrdSpringArm;
	UPROPERTY(VisibleAnywhere, Category = Components)
		UCameraComponent* FirstCamera;
	UPROPERTY(VisibleAnywhere, Category = Components)
		UCameraComponent* ThrdCamera;
	UPROPERTY(VisibleAnywhere, Category = Components)
		UTrackComponent* LeftTrack;
	UPROPERTY(VisibleAnywhere, Category = Components)
		UTrackComponent* RightTrack;
	UPROPERTY(VisibleAnywhere, Category = Components)
		UInstancedStaticMeshComponent* LeftTrackMesh;
	UPROPERTY(VisibleAnywhere, Category = Components)
		UInstancedStaticMeshComponent* RightTrackMesh;
	//UPROPERTY(EditDefaultsOnly, Category = Effects)
	//	UVehicleDustType* DustFX;
	//UPROPERTY(EditDefaultsOnly, Category = Effects)
	//	TSubclassOf<AVehicleImpactEffect> ImpactFXs;

	/** camera shake on impact */
	UPROPERTY(Category = Effects, EditDefaultsOnly)
		TSubclassOf<UCameraShake> ImpactCameraShake;

	/** The minimum amount of normal force that must be applied to the chassis to spawn an Impact Effect */
	UPROPERTY(EditAnywhere, Category = Effects)
		float ImpactEffectNormalForceThreshold = 100000;

	UPROPERTY(EditDefaultsOnly, Category = Effects)
		UParticleSystem* DeathVFX;
	UPROPERTY(EditDefaultsOnly, Category = Effects)
		USoundCue * DeathSFX;
	UPROPERTY(EditDefaultsOnly, Category = Effects)
		USoundCue * EngineSFX;
	UPROPERTY(EditDefaultsOnly, Category = Effects)
		USoundCue * EngineIgnitionSFX;
	UPROPERTY(EditDefaultsOnly, Category = Effects)
		USoundCue * EngineFullThrottleSFX;
	UPROPERTY(EditDefaultsOnly, Category = Effects)
		USoundCue * TrackRollingSFX;
	UPROPERTY(EditDefaultsOnly, Category = Effects)
		USoundCue * TurretRotateSFX;
	UPROPERTY(EditDefaultsOnly, Category = Effects)
		float EngineIgnitionDuration = 1;

	UPROPERTY(EditDefaultsOnly, Category = Effects)
		USoundCue * LandingSFX;
	/** The amount of spring compression required during landing to play sound */
	UPROPERTY(Category = Effects, EditDefaultsOnly)
		float SpringCompressionLandingThreshold = 250000;

	UPROPERTY(EditDefaultsOnly, Category = Effects)
		USoundCue * SkiddingSFX;
	UPROPERTY(EditDefaultsOnly, Category = Effects)
		USoundCue * SkidStoppingSFX;
	UPROPERTY(EditDefaultsOnly, Category = Effects)
		float SkidFadeoutTime = 1;
	/** Skid effects cannot play if velocity is lower than this */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
		float SkidSFXSpeedThreshold = 10;
	/** Skit effects will play if absolute value of tire longitudinal slip is more than this. */
	UPROPERTY(EditDefaultsOnly, Category = Effects, meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
		float SkidSFXLongSlipThreshold = 0.8;
	/** Skid effects will play if absolute value of tire lateral slip is more than this. */
	UPROPERTY(EditDefaultsOnly, Category = Effects, meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
		float SkidSFXLateralSlipThreshold = 0.8;
	/** If skidding time is shorter than this value, skid stop sfx won't be played */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
		float SkidStoppingSFXMinSkidDuration = 0.5;

	/** track effects cannot play if velocity is lower than this */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
		float TrackRollingSFXSpeedThreshold = 15;
	UPROPERTY(EditDefaultsOnly, Category = Effects)
		float TrackRollingSfxFadeoutTime = 1;

	/** Camouflage time penalty received when firing with camouflage on */
	UPROPERTY(EditDefaultsOnly, Category = Camouflage)
		float FiringCamouflageDurationPenalty = 5;

	/** Camouflage time penalty received when getting hit with camouflage on */
	UPROPERTY(EditDefaultsOnly, Category = Camouflage)
		float GetHitCamouflageDurationPenalty = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadonly, Category = Health)
		int Hitpoint = 1000;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Health)
		bool bCanDie = true;
	
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components)
		USkeletalMeshComponent * ChassisMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		UTankCameraMovementComponent * CameraMovementComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UTankMainWeaponComponent * MainWeaponComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		UTankMovementComponent * MovementComponent;
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	//	UCamouflageComponent * CamouflageComponent;
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	//	UTankSpottingComponent * SpottingComponent;

	/** Identifies if pawn is in its dying state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Health, ReplicatedUsing = OnReplicate_Dying)
		bool bIsDying = false;
	UPROPERTY(EditAnywhere, Category = Login)
	float fDetectRadius;
	UPROPERTY(EditAnywhere, Category = Login)
	bool bFirstPersonView = false;

public:
	///implement IVehicle
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "MyCategory")
	void MoveForwordImpl(float forward, float right);
	virtual void MoveForwordImpl_Implementation(float forward, float right) override;
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "MyCategory")
	void MoveRightImpl(float forward, float right);
	virtual void MoveRightImpl_Implementation(float forward, float right) override;
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MyCategory")
	bool DetectInArea(AActor* enterActor);
	virtual bool DetectInArea_Implementation(AActor* enterActor) override;
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "MyCategory")
	void AimAzimuth(float value);
	void AimAzimuth_Implementation(float value) override;
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "MyCategory")
	void AimElevation(float value);
	void AimElevation_Implementation(float value) override;
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "MyCategory")
	void ZoomIn();
	void ZoomIn_Implementation() override;
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "MyCategory")
	void ZoomOut();
	void ZoomOut_Implementation() override;

	UFUNCTION(BlueprintImplementableEvent)
	void MoveBpForward();
	UFUNCTION(BlueprintImplementableEvent)
	void MoveBpRight();

protected:
	// Begin Actor overrides
	void PostInitializeComponents() override;
	void BeginPlay() override;
	void Tick(float deltaTime) override;
	void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalForce, const FHitResult& Hit) override;
	void FellOutOfWorld(const UDamageType& dmgType) override;
	// End Actor overrides

	// Begin Pawn overrides
	void DisplayDebug(class UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;
	void SetupPlayerInputComponent(class UInputComponent* playerInputComponent) override;
	float TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	void TornOff() override;
	void UnPossessed() override;
	// End Pawn overrides

	/** Returns True if the pawn can die in the current state */
	virtual bool CanDie() const;

	/** Kills pawn. [Server/authority only] */
	virtual void Die();

	/** Event on death [Server/Client] */
	virtual void OnDeath();

	/** Replicating death on client */
	UFUNCTION()
		void OnReplicate_Dying();

	/** Update effects under wheels */
	void UpdateWheelEffects();

	void InitProperties();

	void SwitchCamera(bool bFirstCamera);

public:
	ATank();
	//UFUNCTION(BlueprintImplementableEvent)
	//	void MoveForword(float forward);
	//UFUNCTION(BlueprintImplementableEvent)
	//	void MoveRight(float right);

	UFUNCTION(BlueprintCallable)
		FVector GetAiTargetLocation() const;
	UFUNCTION(BlueprintCallable)
		void SetHighlight(bool bHighlight);
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "SetHighlight"))
		void ReceiveSetHighlight(bool bHighlight);

	UFUNCTION(BlueprintCallable)
		float GetHullAlignment() const;
	UFUNCTION(BlueprintCallable)
		float GetTurretAlignment() const;
	UFUNCTION(BlueprintCallable)
		FORCEINLINE int GetRemainingHitpoint() const { return RemainingHitpoint; };

	UFUNCTION(BlueprintCallable)
	bool TryFireGun();

	UFUNCTION(BlueprintCallable)
	void ZoomCamera(int inc);
};

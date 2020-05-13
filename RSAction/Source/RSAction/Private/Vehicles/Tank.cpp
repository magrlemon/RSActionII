// Fill out your copyright notice in the Description page of Project Settings.

#include "Tank.h"

//#include "CamouflageComponent.h"
//#include "VehicleDustType.h"
#include "TankCameraMovementComponent.h"
#include "TankMainWeaponComponent.h"
#include "TankMovementComponent.h"
//#include "TankSpottingComponent.h"
//#include "VehicleEngineSoundNode.h"
#include "VehicleImpactEffect.h"
#include "SoldierGameInstance.h"
#include "AudioThread.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DisplayDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UnrealNetwork.h"
#include "TrackComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
//#include "TankPlayerController.h"
#include "Player/SoldierCharacter.h"

// Sets default values
ATank::ATank() : Super()
{
	PrimaryActorTick.bCanEverTick = true;
	
	InitProperties();
		
	//m_Controller = NewObject<ATankPlayerController>();
	//Controller = m_Controller;

	ChassisMesh = CreateDefaultSubobject<USkeletalMeshComponent>("Chassis");
	ChassisMesh->SetCollisionProfileName(UCollisionProfile::Vehicle_ProfileName);
	ChassisMesh->BodyInstance.bSimulatePhysics = true;
	ChassisMesh->BodyInstance.bNotifyRigidBodyCollision = true;
	ChassisMesh->BodyInstance.bUseCCD = true;
	ChassisMesh->bBlendPhysics = true;
	ChassisMesh->SetGenerateOverlapEvents(true);
	ChassisMesh->SetCanEverAffectNavigation(true);
	RootComponent = ChassisMesh;

	MainWeaponComponent = CreateDefaultSubobject<UTankMainWeaponComponent>(FName("TankMainWeaponComponent"));
	CameraMovementComponent = CreateDefaultSubobject<UTankCameraMovementComponent>(FName("TankCameraMovementComponent"));	
	/*SpottingComponent = CreateDefaultSubobject<UTankSpottingComponent>(FName("TankSpottingComponent"));
	CamouflageComponent = CreateDefaultSubobject<UCamouflageComponent>(FName("TankCamouflageComponent"));*/

	MovementComponent = CreateDefaultSubobject<UTankMovementComponent>(FName("TankMovementComponent"));
	MovementComponent->SetIsReplicated(true);
	MovementComponent->UpdatedComponent = ChassisMesh;

	TurretComponent = CreateDefaultSubobject<UStaticMeshComponent>(FName("Turret"));
	TurretComponent->SetRelativeLocation(FVector(0, 0, 0));
	TurretComponent->AttachTo(RootComponent);
	//BarrelComponent = CreateDefaultSubobject<USkeletalMeshComponent>(FName("Barral"));
	//BarrelComponent->AttachTo(RootComponent);
	//BarrelComponent->SetRelativeLocation(FVector(0, 0, 0));

	RootSpringArm = CreateDefaultSubobject<USpringArmComponent>(FName("RootSprintArm"));
	RootSpringArm->SetRelativeLocation(FVector(0, 0, 0));
	RootSpringArm->AttachTo(RootComponent);
	FirstSpringArm = CreateDefaultSubobject<USpringArmComponent>(FName("FirstSprintArm"));
	FirstSpringArm->SetRelativeLocation(FVector(0,0,0));
	FirstSpringArm->AttachTo(RootSpringArm);
	ThrdSpringArm = CreateDefaultSubobject<USpringArmComponent>(FName("SecondSprintArm"));
	ThrdSpringArm->SetRelativeLocation(FVector(0, 0, 0));
	ThrdSpringArm->AttachTo(RootSpringArm);
	FirstCamera = CreateDefaultSubobject<UCameraComponent>(FName("FirstCamera"));
	FirstCamera->AttachTo(FirstSpringArm);
	ThrdCamera = CreateDefaultSubobject<UCameraComponent>(FName("SecondCamera"));
	ThrdCamera->AttachTo(ThrdSpringArm);

	//LeftTrack = CreateDefaultSubobject<UTrackComponent>(FName("LeftTrack"));
	//LeftTrack->AttachTo(RootComponent);
	//LeftTrack->SetRelativeLocation(FVector(0, 0, 0));
	//RightTrack = CreateDefaultSubobject<UTrackComponent>(FName("RightTrack"));
	//RightTrack->AttachTo(RootComponent);
	//RightTrack->SetRelativeLocation(FVector(0, 0, 0));
	//LeftTrackMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(FName("LeftTrackMesh"));
	//LeftTrackMesh->AttachTo(LeftTrack);
	//RightTrackMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(FName("RightTrackMesh"));
	//RightTrackMesh->AttachTo(RightTrack);

	EngineAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("EngineAudio"));
	EngineAudioComponent->SetupAttachment(ChassisMesh);

	SkidAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("SkidAudio"));
	SkidAudioComponent->bAutoActivate = false;
	SkidAudioComponent->SetupAttachment(ChassisMesh);

	TrackRollingAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("TrackRollingAudio"));
	TrackRollingAudioComponent->bAutoActivate = false;
	TrackRollingAudioComponent->SetupAttachment(ChassisMesh);

	FullThrottleRotateAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("FullThrottleRotateAudio"));
	FullThrottleRotateAudioComponent->bAutoActivate = false;
	FullThrottleRotateAudioComponent->SetupAttachment(ChassisMesh);

	TurretRotateAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("TurretRotateAudio"));
	TurretRotateAudioComponent->bAutoActivate = false;
	TurretRotateAudioComponent->SetupAttachment(ChassisMesh);
	MainWeaponComponent->SetTurretRotateAudioComponent(TurretRotateAudioComponent);

	AITarget = CreateDefaultSubobject<USceneComponent>(TEXT("AITarget"));
	AITarget->SetupAttachment(ChassisMesh);
}

void ATank::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (EngineAudioComponent)
	{
		EngineAudioComponent->SetSound(EngineSFX);
		EngineAudioComponent->Play();
	}

	if (SkidAudioComponent)
	{
		SkidAudioComponent->SetSound(SkiddingSFX);
		SkidAudioComponent->Stop();
	}

	if(TurretRotateAudioComponent)
	{
		TurretRotateAudioComponent->SetSound(TurretRotateSFX);
		TurretRotateAudioComponent->Stop();
	}

	//if (Controller == NULL)
	//	Controller = NewObject<ATankPlayerController>(this, ATankPlayerController::StaticClass());

	// Full throttle sfx only need to be played on player controlled tank
	if(Cast<APlayerController>(Controller))
	{
		if (FullThrottleRotateAudioComponent)
		{
			FullThrottleRotateAudioComponent->SetSound(EngineFullThrottleSFX);
			FullThrottleRotateAudioComponent->Stop();
		}

		if (TrackRollingAudioComponent)
		{
			TrackRollingAudioComponent->SetSound(TrackRollingSFX);
			TrackRollingAudioComponent->Stop();
		}
	}

	//CamouflageComponent->OnChangeCamouflage.AddDynamic(SpottingComponent, &UTankSpottingComponent::SetInvisibleModeFactor);
	RemainingHitpoint = Hitpoint;
	
}
void ATank::InitBarrel(USkeletalMeshComponent* barrel)
{
	BarrelComponent = barrel;
}
void ATank::BeginPlay()
{
	Super::BeginPlay();

	InitBP();
		
	DustParticleComponents.SetNum(MovementComponent->Wheels.Num());

	//if (m_Controller != NULL)
	//Controller = NewObject<ATankPlayerController>(this, ATankPlayerController::StaticClass()); //m_Controller->GetClass()->GetDefaultObject<ATankPlayerController>();

	// Ignite engine
	if (EngineIgnitionSFX && Cast<APlayerController>(Controller))
	{
		UGameplayStatics::PlaySoundAtLocation(this, EngineIgnitionSFX, GetActorLocation(), GetActorRotation(), 1, 1, 0, nullptr, nullptr, this);

		if(const auto player = Cast<APlayerController>(Controller))
		{
			DisableInput(player);
			
			FTimerHandle timerHandle;
			GetWorld()->GetTimerManager().SetTimer(timerHandle, [this, player]
			{
				EnableInput(player);
			}, EngineIgnitionDuration, false);
		}
	}

	MainWeaponComponent->Init(TurretComponent, BarrelComponent);
	/*MovementComponent->Init(LeftTrack, RightTrack);
	LeftTrack->BuildTrack(LeftTrackMesh);
	RightTrack->BuildTrack(RightTrackMesh);*/
	CameraMovementComponent->Init(FirstCamera, FirstSpringArm, ThrdSpringArm);
	SwitchCamera(false);

	FirstSpringArm->SetWorldRotation(TurretComponent->GetComponentRotation());
	ThrdSpringArm->SetWorldRotation(TurretComponent->GetComponentRotation());
	RootSpringArm->SetWorldRotation(TurretComponent->GetComponentRotation());	
}

void ATank::Tick(float deltaTime)
{
	Super::Tick(deltaTime);	

	AimTowardCusor();
	UpdateBarrelCursor();
	
	//UpdateWheelEffects();

	//// Play full-throttle sfx
	//const auto currentThrottle = MovementComponent->GetThrottleInput();
	//if(currentThrottle == 0)
	//{
	//	bFullThrottleSfxPlayed = false;
	//	FullThrottleRotateAudioComponent->FadeOut(1, 0);
	//}

	//if(!bFullThrottleSfxPlayed && currentThrottle > 0)
	//{
	//	bFullThrottleSfxPlayed = true;
	//	FullThrottleRotateAudioComponent->Play();
	//}

	//// Set engine rpm param for sfx
	//if (Controller)
	//{
	//	const auto playerId = Controller->GetUniqueID();

	//	UVehicleEngineSoundNode::FVehicleDesiredRPM desiredRpm;
	//	desiredRpm.DesiredRPM = FMath::Abs(MovementComponent->GetEngineRotationSpeed());
	//	desiredRpm.TimeStamp = GetWorld()->GetTimeSeconds();

	//	// TODO Use which one again??
	//	{
	//		UVehicleEngineSoundNode::SetDesiredRPM(playerId, desiredRpm);
	//		EngineAudioComponent->SetFloatParameter(FName("RPM"), desiredRpm.DesiredRPM);
	//	}
	//}

	// Flag to reset highlight next frame
	if(bHighlighting)
	{
		bHighlighting = false;
	}
	else
	{
		SetHighlight(bHighlighting);
	}
}

void ATank::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved,
	FVector HitLocation, FVector HitNormal, FVector NormalForce, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalForce, Hit);

	// Spawn impact fx
	if (ImpactFXs && NormalForce.SizeSquared() > FMath::Square(ImpactEffectNormalForceThreshold))
	{
		const FTransform spawnTransform(HitNormal.Rotation(), HitLocation);
		auto effectActor = GetWorld()->SpawnActorDeferred<AVehicleImpactEffect>(ImpactFXs, spawnTransform);
		if (effectActor)
		{
			effectActor->HitSurface = Hit;
			effectActor->HitForce = NormalForce;
			// TODO: Magic number here
			effectActor->bWheelLand = FVector::DotProduct(HitNormal, GetActorUpVector()) > 0.8f;

			UGameplayStatics::FinishSpawningActor(effectActor, spawnTransform);
		}
	}

	// Shake cam
	if (ImpactCameraShake)
	{
		auto player = Cast<APlayerController>(Controller);
		if (player && player->IsLocalController())
		{
			player->ClientPlayCameraShake(ImpactCameraShake, 1);
		}
	}
}

void ATank::FellOutOfWorld(const UDamageType & dmgType)
{
	Die();
}


void ATank::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	static FName NAME_Vehicle = FName(TEXT("Vehicle"));

	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

	if (DebugDisplay.IsDisplayOn(NAME_Vehicle))
	{
		MovementComponent->DrawDebug(Canvas, YL, YPos);
	}
}

void ATank::SetupPlayerInputComponent(UInputComponent* playerInputComponent)
{
	//Super::SetupPlayerInputComponent(playerInputComponent);

	playerInputComponent->BindAxis("MoveForward", this, &ATank::MoveForward);
	playerInputComponent->BindAxis("MoveRight", this, &ATank::MoveRight);
	playerInputComponent->BindAxis("Turn", this, &ATank::AimAzimuth);
	playerInputComponent->BindAxis("LookUp", this, &ATank::AimElevation);
	playerInputComponent->BindAction("Fire", IE_Pressed, this, &ATank::Fire);
	playerInputComponent->BindAction("LoginTank", IE_Pressed, this, &ATank::LogOutTank);
	playerInputComponent->BindAction("ZoomIn", IE_Pressed, this, &ATank::ZoomIn);
	playerInputComponent->BindAction("ZoomOut", IE_Pressed, this, &ATank::ZoomOut);
	playerInputComponent->BindAction("LockBarrel", IE_Pressed, this, &ATank::UnLockBarrel);
	playerInputComponent->BindAction("LockBarrel", IE_Released, this, &ATank::LockBarrel);
}

/* Merge APawn's and AActor's */
float ATank::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!ShouldTakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser))
	{
		return 0.f;
	}

	float ActualDamage = Damage;

	UDamageType const* const DamageTypeCDO = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		// point damage event, pass off to helper function
		FPointDamageEvent* const PointDamageEvent = (FPointDamageEvent*)&DamageEvent;
		ActualDamage = InternalTakePointDamage(ActualDamage, *PointDamageEvent, EventInstigator, DamageCauser);

		RemainingHitpoint -= ActualDamage;

		//if (CamouflageComponent && CamouflageComponent->GetCamouflageFactor() > 0)
		//{
		//	CamouflageComponent->Deplet(GetHitCamouflageDurationPenalty);
		//}

		// K2 notification for this actor
		if (ActualDamage != 0.f)
		{
			ReceivePointDamage(ActualDamage, DamageTypeCDO, PointDamageEvent->HitInfo.ImpactPoint, PointDamageEvent->HitInfo.ImpactNormal, PointDamageEvent->HitInfo.Component.Get(), PointDamageEvent->HitInfo.BoneName, PointDamageEvent->ShotDirection, EventInstigator, DamageCauser, PointDamageEvent->HitInfo);
			OnTakePointDamage.Broadcast(this, ActualDamage, EventInstigator, PointDamageEvent->HitInfo.ImpactPoint, PointDamageEvent->HitInfo.Component.Get(), PointDamageEvent->HitInfo.BoneName, PointDamageEvent->ShotDirection, DamageTypeCDO, DamageCauser);

			// Notify the component
			UPrimitiveComponent* const PrimComp = PointDamageEvent->HitInfo.Component.Get();
			if (PrimComp)
			{
				PrimComp->ReceiveComponentDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
			}
		}
	}
	else if (DamageEvent.IsOfType(FRadialDamageEvent::ClassID))
	{
		// radial damage event, pass off to helper function
		FRadialDamageEvent* const RadialDamageEvent = (FRadialDamageEvent*)&DamageEvent;
		ActualDamage = InternalTakeRadialDamage(ActualDamage, *RadialDamageEvent, EventInstigator, DamageCauser);

		RemainingHitpoint -= ActualDamage;

		//if (CamouflageComponent && CamouflageComponent->GetCamouflageFactor() > 0)
		//{
		//	CamouflageComponent->Deplet(GetHitCamouflageDurationPenalty);
		//}

		// K2 notification for this actor
		if (ActualDamage != 0.f)
		{
			FHitResult const& Hit = (RadialDamageEvent->ComponentHits.Num() > 0) ? RadialDamageEvent->ComponentHits[0] : FHitResult();
			ReceiveRadialDamage(ActualDamage, DamageTypeCDO, RadialDamageEvent->Origin, Hit, EventInstigator, DamageCauser);

			// add any desired physics impulses to our components
			for (int HitIdx = 0; HitIdx < RadialDamageEvent->ComponentHits.Num(); ++HitIdx)
			{
				FHitResult const& CompHit = RadialDamageEvent->ComponentHits[HitIdx];
				UPrimitiveComponent* const PrimComp = CompHit.Component.Get();
				if (PrimComp && PrimComp->GetOwner() == this)
				{
					PrimComp->ReceiveComponentDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
				}
			}
		}
	}

	// generic damage notifications sent for any damage
	// note we will broadcast these for negative damage as well
	if (ActualDamage != 0.f)
	{
		ReceiveAnyDamage(ActualDamage, DamageTypeCDO, EventInstigator, DamageCauser);
		OnTakeAnyDamage.Broadcast(this, ActualDamage, DamageTypeCDO, EventInstigator, DamageCauser);
		if (EventInstigator != NULL)
		{
			EventInstigator->InstigatedAnyDamage(ActualDamage, DamageTypeCDO, this, DamageCauser);
		}
	}

	if (const auto controller = GetController())
	{
		controller->TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	}

	if (RemainingHitpoint <= 0)
	{
		Die();
	}

	return ActualDamage;
}

void ATank::TornOff()
{
	Super::TornOff();

	SetLifeSpan(1);
}

void ATank::UnPossessed()
{
	/*if (Controller)
	{
		const auto playerId = Controller->GetUniqueID();
		UVehicleEngineSoundNode::RemoveDesiredRPM(playerId);
	}*/

	// Super clears controller, so do the behavior first
	Super::UnPossessed();
}



#pragma region Networking

	void ATank::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
	{
		Super::GetLifetimeReplicatedProps(OutLifetimeProps);

		DOREPLIFETIME(ATank, bIsDying);
	}

	void ATank::OnReplicate_Dying()
	{
		if (bIsDying)
		{
			OnDeath();
		}
	}

#pragma endregion Networking


bool ATank::CanDie() const
{
	if (!bCanDie
		|| bIsDying										// already dying
		|| IsPendingKill()								// already destroyed
		|| Role != ROLE_Authority)						// not authority
	{
		return false;
	}

	return true;
}

void ATank::Die()
{
	if (CanDie())
	{
		OnDeath();
	}
}

void ATank::OnDeath()
{
	bReplicateMovement = false;
	//bTearOff = true;
	bIsDying = true;

	DetachFromControllerPendingDestroy();

	// hide and disable
	TurnOff();
	SetActorHiddenInGame(true);

	if (EngineAudioComponent)
	{
		EngineAudioComponent->Stop();
	}

	if (SkidAudioComponent)
	{
		SkidAudioComponent->Stop();
	}

	if (DeathVFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, DeathVFX, GetActorLocation(), GetActorRotation());
	}

	if (DeathSFX)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathSFX, GetActorLocation());
	}

	// Finite lifespan so that auto destroying can kick in
	SetLifeSpan(0.2f);
}

void ATank::UpdateWheelEffects()
{
	//const auto bOnAirLastFrame = bOnAir;
	//bOnAir = true;
	//const auto currentSpeed = MovementComponent->GetForwardSpeed();

	//// Spawn dust vfx
	//if (DustFX && !bIsDying && MovementComponent->Wheels.Num() > 0)
	//{
	//	for (auto i = 0; i < DustParticleComponents.Num(); i++)
	//	{
	//		const auto contactMat = MovementComponent->Wheels[i]->GetContactSurfaceMaterial();
	//		const auto dustParticle = DustFX->GetDustFX(contactMat, currentSpeed);

	//		// Considered on air if all wheels doesnt come into contact with surface
	//		if (contactMat != nullptr)
	//		{
	//			bOnAir = false;
	//		}

	//		auto dustParticleComponent = DustParticleComponents[i];
	//		const auto bIsActive = dustParticleComponent && !dustParticleComponent->bWasDeactivated && !dustParticleComponent->bWasCompleted;
	//		const auto currentDustParticle = dustParticleComponent ? dustParticleComponent->Template : nullptr;

	//		if (dustParticle && (currentDustParticle != dustParticle || !bIsActive))
	//		{
	//			// Replace old particle system component before applying new particle
	//			if (dustParticleComponent == nullptr || !dustParticleComponent->bWasDeactivated)
	//			{
	//				if (dustParticleComponent != nullptr)
	//				{
	//					dustParticleComponent->SetActive(false);
	//					dustParticleComponent->bAutoDestroy = true;
	//				}

	//				DustParticleComponents[i] = dustParticleComponent = NewObject<UParticleSystemComponent>(this);
	//				dustParticleComponent->bAutoActivate = true;
	//				dustParticleComponent->bAutoDestroy = false;
	//				dustParticleComponent->RegisterComponentWithWorld(GetWorld());
	//				dustParticleComponent->AttachToComponent(ChassisMesh, FAttachmentTransformRules::KeepRelativeTransform, MovementComponent->WheelSetups[i].BoneName);
	//			}

	//			dustParticleComponent->SetTemplate(dustParticle);
	//			dustParticleComponent->ActivateSystem();
	//		}
	//		else if (!dustParticle && bIsActive)
	//		{
	//			dustParticleComponent->SetActive(false);
	//		}
	//	}
	//}

	/*if (bOnAirLastFrame && !bOnAir && LandingSFX)
	{
		if (MovementComponent->GetMaxSpringForce() > SpringCompressionLandingThreshold)
		{
			UGameplayStatics::PlaySoundAtLocation(this, LandingSFX, GetActorLocation());
		}
	}*/

	//// Add skidding sfx
	//if (SkidAudioComponent)
	//{
	//	const auto bVehicleStopped = currentSpeed < SkidSFXSpeedThreshold;
	//	const auto bTireSlipping = MovementComponent->CheckSlipThreshold(SkidSFXLongSlipThreshold, SkidSFXLateralSlipThreshold);
	//	const auto bShallSkid = !bOnAir && !bVehicleStopped && bTireSlipping;

	//	const auto currentTime = GetWorld()->GetTimeSeconds();
	//	
	//	if (bShallSkid && !bSkidding)
	//	{
	//		bSkidding = true;
	//		SkidAudioComponent->Play();
	//		SkidStartTime = currentTime;
	//	}
	//	
	//	else if (!bShallSkid && bSkidding)
	//	{
	//		bSkidding = false;
	//		SkidAudioComponent->FadeOut(SkidFadeoutTime, 0);
	//		if (currentTime - SkidStartTime > SkidStoppingSFXMinSkidDuration)
	//		{
	//			UGameplayStatics::PlaySoundAtLocation(this, SkidStoppingSFX, GetActorLocation());
	//		}
	//	}
	//}

	//// Add track rolling sfx
	//if(TrackRollingAudioComponent)
	//{
	//	// TODO: Hard code the name here
	//	TrackRollingAudioComponent->SetFloatParameter("Speed", currentSpeed);

	//	if(currentSpeed < TrackRollingSFXSpeedThreshold)
	//	{
	//		if(bTrackRollingSfxPlaying)
	//		{
	//			TrackRollingAudioComponent->FadeOut(TrackRollingSfxFadeoutTime, 0);
	//			bTrackRollingSfxPlaying = false;
	//		}
	//	} 
	//	else
	//	{
	//		if(!bTrackRollingSfxPlaying)
	//		{
	//			TrackRollingAudioComponent->Play();
	//			bTrackRollingSfxPlaying = true;
	//		}
	//	}
	//}
}




FVector ATank::GetAiTargetLocation() const
{
	return AITarget->GetComponentLocation();
}

void ATank::SetHighlight(bool bHighlight)
{
	bHighlighting = bHighlight;

	ReceiveSetHighlight(bHighlight);
}


float ATank::GetHullAlignment() const
{
	const auto hullYawVector = FVector::VectorPlaneProject(GetActorForwardVector(), CameraMovementComponent->GetCameraUpVector()).GetSafeNormal();
	const auto angle = FMath::RadiansToDegrees(FMath::Acos(hullYawVector | CameraMovementComponent->GetCameraForwardVector()));
	return (hullYawVector | CameraMovementComponent->GetCameraRightVector()) > 0 ? angle : -angle;
}

float ATank::GetTurretAlignment() const
{	
	const auto turretYawVector = FVector::VectorPlaneProject(MainWeaponComponent->GetTurretForwardVector(), CameraMovementComponent->GetCameraUpVector()).GetSafeNormal();
	auto angle = FMath::RadiansToDegrees(FMath::Acos(turretYawVector | CameraMovementComponent->GetCameraForwardVector()));
	return (turretYawVector | CameraMovementComponent->GetCameraRightVector()) > 0 ? angle : -angle;
}

bool ATank::TryFireGun()
{
	if (MainWeaponComponent->TryFireGun())
	{
		/*SpottingComponent->bHasFired = true;

		if (CamouflageComponent && CamouflageComponent->GetCamouflageFactor() > 0)
		{
			CamouflageComponent->Deplet(FiringCamouflageDurationPenalty);
		}*/

		return true;
	}

	return false;
}
void ATank::InitProperties()
{
	fDetectRadius = 300;
}

void ATank::MoveForward(float value)
{
	float right = GetInputAxisValue("MoveRight");
	float input = value < 0 ? -1 : 1;
	MovementComponent->SetThrottleInput(input * (FMath::Pow(value, 2) + FMath::Pow(right, 2)) * 10);
}

void ATank::MoveRight(float value)
{
	float forward = GetInputAxisValue("MoveForward");
	FVector2D dir(forward, value);
	MovementComponent->SetSteeringDirection(dir);
}



void ATank::AimAzimuth(float value)
{
	if (CameraMovementComponent != NULL)
	{
		CameraMovementComponent->RotateCameraYaw(value * 5);
	}
}

void ATank::AimElevation(float value)
{
	if (CameraMovementComponent != NULL)
	{
		CameraMovementComponent->RotateCameraPitch(value * 5);
	}
}

void ATank::ZoomIn()
{
	ZoomCamera(1);
}
void ATank::ZoomOut()
{
	ZoomCamera(-1);
}

void ATank::ZoomCamera(int inc)
{
	if(bFirstPersonView)
	{
		ZoomIndex = CameraMovementComponent->TargetFirstPersonZoomLevelIndex + inc;
		if(ZoomIndex < 0)
		{
			CameraMovementComponent->SetThirdPersonZoomStep(0);
			SwitchCamera(false);
		}
		else
		{
			CameraMovementComponent->SetFirstPersonZoomLevel(ZoomIndex);
		}
	}
	else
	{
		ZoomIndex = CameraMovementComponent->TargetThirdPersonZoomStepIndex - inc;
		if (ZoomIndex < 0)
		{
			CameraMovementComponent->SetFirstPersonZoomLevel(0);
			SwitchCamera(true);
		}
		else
		{
			CameraMovementComponent->SetThirdPersonZoomStep(ZoomIndex);
		}
	}
}

void ATank::SwitchCamera(bool bFirstCamera)
{
	bFirstPersonView = bFirstCamera;
	FirstCamera->SetActive(bFirstCamera);
	ThrdCamera->SetActive(!bFirstCamera);
}

void ATank::Fire()
{
	TryFireGun();
}

bool ATank::DetectInArea_Implementation(AActor* enterActor)
{
	float dist = FVector::Dist(this->GetActorLocation(), enterActor->GetActorLocation());
	return !(dist > fDetectRadius);
}

void ATank::EnableVehicleInput_Implementation(ASoldierCharacter* FirstPersonChr, APlayerController* ctr)
{
	m_SavedCtrl.InputYawScale = ctr->InputYawScale;
	m_SavedCtrl.InputPitchScale = ctr->InputPitchScale;
	m_SavedCtrl.InputRollScale = ctr->InputRollScale;
	m_SavedCtrl.SmoothTargetViewRotationSpeed = ctr->SmoothTargetViewRotationSpeed;

	ctr->InputYawScale = 2.5f;
	ctr->InputPitchScale = -2.5f;
	ctr->InputRollScale = 1;
	ctr->SmoothTargetViewRotationSpeed = 20;
	
	Controller = ctr;
	m_Crew = FirstPersonChr;
	
	EnablePhysicMass(true);

	USoldierGameInstance* sGameInstance = StaticCast<USoldierGameInstance*>(GWorld->GetGameInstance());
	if (sGameInstance->IsLoginVehicle())
	{
		Controller->Possess(this);
	}
}

void ATank::LogOutTank()
{
	m_Crew->Controller = Controller;

	Cast<APlayerController>(m_Crew->Controller)->InputYawScale = m_SavedCtrl.InputYawScale;
	Cast<APlayerController>(m_Crew->Controller)->InputPitchScale = m_SavedCtrl.InputPitchScale;
	Cast<APlayerController>(m_Crew->Controller)->InputRollScale = m_SavedCtrl.InputRollScale;
	Cast<APlayerController>(m_Crew->Controller)->SmoothTargetViewRotationSpeed = m_SavedCtrl.SmoothTargetViewRotationSpeed;

	EnablePhysicMass(false);

	m_Crew->LoginTank();
}

void ATank::LockBarrel()
{
	MainWeaponComponent->bLockGun = true;
}

void ATank::UnLockBarrel()
{
	MainWeaponComponent->bLockGun = false;
}
void ATank::UpdateBarrelCursor()
{
	FPredictProjectilePathResult result;
	MainWeaponComponent->TraceProjectilePath(result);
	
	if(result.HitResult.bBlockingHit)
	{
		MainWeaponComponent->SetCursorLocation(Cast<APlayerController>(Controller), result.HitResult.Location);
	}
	else
	{
		MainWeaponComponent->SetCursorLocation(Cast<APlayerController>(Controller),  result.LastTraceDestination.Location);
	}
}

void ATank::AimTowardCusor()
{
	if (Controller && !MainWeaponComponent->bLockGun)// 
	{
		FVector targetPos,wLoc,wDir;
		int32 sizex, sizey;		
		APlayerController* pController = Cast<APlayerController>(Controller);
		if (pController)
		{
			pController->GetViewportSize(sizex, sizey);
			pController->DeprojectScreenPositionToWorld(sizex*0.5, sizey*0.5, wLoc, wDir);

			GetAimingTargetPosition(wLoc, wDir, AimingLineTraceRange, targetPos);
			MainWeaponComponent->AimGun(wLoc, targetPos,false);
			
		}
	}
}

void ATank::GetAimingTargetPosition(FVector const &CursorWorldLocation, FVector const &CursorWorldDirection, float const LineTraceRange, FVector &OutTargetPosition) const
{
	// Safe-guard in case the cursor's camera clip through a wall behind or something.
	float const LINE_TRACE_START_DISTANCE_FROM_CURSOR = 500;

	FHitResult outHitresult;
	const auto lineTraceStartPos = CursorWorldLocation + CursorWorldDirection * LINE_TRACE_START_DISTANCE_FROM_CURSOR;
	const auto lineTraceEndPos = lineTraceStartPos + CursorWorldDirection * LineTraceRange;

	auto collisionQueryParams = FCollisionQueryParams();	
	{
		collisionQueryParams.AddIgnoredActor(this);
	}

	if (GetWorld()->LineTraceSingleByChannel(outHitresult, lineTraceStartPos, lineTraceEndPos, ECollisionChannel::ECC_Camera, collisionQueryParams))
	{
		OutTargetPosition = outHitresult.Location;

		// If hit a tank, highlight it
		if (outHitresult.Actor.IsValid())
		{
			if (auto hitTank = Cast<ATank>(outHitresult.Actor.Get()))
			{
				hitTank->SetHighlight(true);
			}
		}
	}
	else
	{
		OutTargetPosition = lineTraceEndPos;
	}
}
// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "RSAction.h"
#include "Weapons/SoldierWeapon.h"
#include "Weapons/SoldierDamageType.h"
#include "UI/SoldierHUD.h"
#include "Online/SoldierPlayerState.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Sound/SoundNodeLocalPlayer.h"
#include "AudioThread.h"

static int32 NetVisualizeRelevancyTestPoints = 0;
FAutoConsoleVariableRef CVarNetVisualizeRelevancyTestPoints(
	TEXT("p.NetVisualizeRelevancyTestPoints"),
	NetVisualizeRelevancyTestPoints,
	TEXT("")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);


static int32 NetEnablePauseRelevancy = 1;
FAutoConsoleVariableRef CVarNetEnablePauseRelevancy(
	TEXT("p.NetEnablePauseRelevancy"),
	NetEnablePauseRelevancy,
	TEXT("")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);

FOnSoldierCharacterEquipWeapon ASoldierCharacter::NotifyEquipWeapon;
FOnSoldierCharacterUnEquipWeapon ASoldierCharacter::NotifyUnEquipWeapon;

ASoldierCharacter::ASoldierCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<USoldierCharacterMovement>(ACharacter::CharacterMovementComponentName))
{
	Mesh1P = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("PawnMesh1P"));
	Mesh1P->SetupAttachment(GetCapsuleComponent());
	Mesh1P->bOnlyOwnerSee = true;
	Mesh1P->bOwnerNoSee = false;
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->bReceivesDecals = false;
	Mesh1P->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	Mesh1P->PrimaryComponentTick.TickGroup = TG_PrePhysics;
	Mesh1P->SetCollisionObjectType(ECC_Pawn);
	Mesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh1P->SetCollisionResponseToAllChannels(ECR_Ignore);

	GetMesh()->bOnlyOwnerSee = false;
	GetMesh()->bOwnerNoSee = true;
	GetMesh()->bReceivesDecals = false;
	GetMesh()->SetCollisionObjectType(ECC_Pawn);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	TargetingSpeedModifier = 0.5f;
	bIsTargeting = false;
	RunningSpeedModifier = 1.5f;
	bWantsToRun = false;
	bWantsToFire = false;
	LowHealthPercentage = 0.5f;

	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;
	m_ECameraMode = ESoldierCamreMode::E_CameraMode_None;
	m_CameraDistance = 300.0f;

	m_ZoomBlendStartDis = 0.f;
	m_ZoomBlendEndDis = 0.f;
	m_ZoomBlendTimeToGo = 0.f;

	m_SwitchBlendStartLoc = FVector(0.f, 0.f, 0.f);
	m_SwitchBlendStartRot = FRotator(0.f, 0.f, 0.f);
	m_SwitchBlendTimeToGo = 0.f;

	m_CameraBlendTime = 1.f;
	m_CameraBlendExp = 2.f;

	bWantsToCrouch = false;
	bWantsToProne = false;

	m_bMesh3P = false;
}

void ASoldierCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (GetLocalRole() == ROLE_Authority)
	{
		Health = GetMaxHealth();

		// Needs to happen after character is added to repgraph
		GetWorldTimerManager().SetTimerForNextTick(this, &ASoldierCharacter::SpawnDefaultInventory);
	}

	// set initial mesh visibility (3rd person view)
	UpdatePawnMeshes();

	// create material instance for setting team colors (3rd person view)
	for (int32 iMat = 0; iMat < GetMesh()->GetNumMaterials(); iMat++)
	{
		MeshMIDs.Add(GetMesh()->CreateAndSetMaterialInstanceDynamic(iMat));
	}

	// play respawn effects
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (RespawnFX)
		{
			UGameplayStatics::SpawnEmitterAtLocation(this, RespawnFX, GetActorLocation(), GetActorRotation());
		}

		if (RespawnSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, RespawnSound, GetActorLocation());
		}
	}
}

void ASoldierCharacter::BeginPlay() {
	Super::BeginPlay();

}

void ASoldierCharacter::Destroyed()
{
	Super::Destroyed();
	DestroyInventory();
}

void ASoldierCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	// switch mesh to 1st person view
	UpdatePawnMeshes();

	// reattach weapon if needed
	SetCurrentWeapon(CurrentWeapon);

	// set team colors for 1st person view
	UMaterialInstanceDynamic* Mesh1PMID = Mesh1P->CreateAndSetMaterialInstanceDynamic(0);
	UpdateTeamColors(Mesh1PMID);

}

void ASoldierCharacter::PossessedBy(class AController* InController)
{
	Super::PossessedBy(InController);

	// [server] as soon as PlayerState is assigned, set team colors of this pawn for local player
	UpdateTeamColorsAllMIDs();
}

void ASoldierCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// [client] as soon as PlayerState is assigned, set team colors of this pawn for local player
	if (GetPlayerState() != NULL)
	{
		UpdateTeamColorsAllMIDs();
	}
}

FRotator ASoldierCharacter::GetAimOffsets() const
{
	const FVector AimDirWS = GetBaseAimRotation().Vector();
	const FVector AimDirLS = ActorToWorld().InverseTransformVectorNoScale(AimDirWS);
	const FRotator AimRotLS = AimDirLS.Rotation();

	return AimRotLS;
}

bool ASoldierCharacter::IsEnemyFor(AController* TestPC) const
{
	if (TestPC == Controller || TestPC == NULL)
	{
		return false;
	}

	ASoldierPlayerState* TestPlayerState = Cast<ASoldierPlayerState>(TestPC->PlayerState);
	ASoldierPlayerState* MyPlayerState = Cast<ASoldierPlayerState>(GetPlayerState());

	bool bIsEnemy = true;
	if (GetWorld()->GetGameState())
	{
		const ASoldierGameMode* DefGame = GetWorld()->GetGameState()->GetDefaultGameMode<ASoldierGameMode>();
		if (DefGame && MyPlayerState && TestPlayerState)
		{
			bIsEnemy = DefGame->CanDealDamage(TestPlayerState, MyPlayerState);
		}
	}

	return bIsEnemy;
}

//////////////////////////////////////////////////////////////////////////
// Meshes

void ASoldierCharacter::UpdatePawnMeshes()
{
	bool const bFirstPerson = IsFirstPerson();
	Mesh1P->VisibilityBasedAnimTickOption = !bFirstPerson ? EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered : EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Mesh1P->SetOwnerNoSee(!bFirstPerson);

	GetMesh()->VisibilityBasedAnimTickOption = bFirstPerson ? EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered : EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	GetMesh()->SetOwnerNoSee(bFirstPerson);

	if (GetWeapon())
		GetWeapon()->SwapMesh1P3PWeaponHidden();
}

void ASoldierCharacter::UpdateTeamColors(UMaterialInstanceDynamic* UseMID)
{
	if (UseMID)
	{
		ASoldierPlayerState* MyPlayerState = Cast<ASoldierPlayerState>(GetPlayerState());
		if (MyPlayerState != NULL)
		{
			float MaterialParam = (float)MyPlayerState->GetTeamNum();
			UseMID->SetScalarParameterValue(TEXT("Team Color Index"), MaterialParam);
		}
	}
}

void ASoldierCharacter::CalcCamera(float DeltaTime, struct FMinimalViewInfo& OutResult) {
	FVector CameraLoc(0.f, 0.f, 0.f);
	FRotator CameraRot(0.f, 0.f, 0.f);
	FVector Pos(0.f, 0.f, 0.f);

	switch (m_ECameraMode)
	{
	case ESoldierCamreMode::E_CameraMode_First:
		GetActorEyesViewPoint(CameraLoc, CameraRot);
		UpdateMesh1PCameraTransform(CameraLoc, CameraRot);
		CameraLoc = GetTransform().TransformPosition(FVector(0.0f, -10.0f,80.0f)) ;
		Pos = CameraLoc;
		break;
	case ESoldierCamreMode::E_CameraMode_Third:
		CameraRot = GetController()->GetControlRotation(); 
		CameraLoc = GetActorLocation();
		Pos = CameraLoc - CameraRot.Vector()*m_CameraDistance + FVector(0.0f, 0.0f, 80.0f);
		break;
	case ESoldierCamreMode::E_CameraMode_ThirdRot:

		CameraRot = GetController()->GetControlRotation();
		CameraLoc = GetTransform().TransformPosition(FVector(0.0f, 100.0f, 80.0f));
		Pos = CameraLoc - CameraRot.Vector()*m_CameraDistance ;
		break;
	case ESoldierCamreMode::E_CameraMode_Top:
		Pos.X = 0.f;
		Pos.Y = 0.f;
		Pos.Z = m_CameraDistance;
		CameraLoc = GetActorLocation();
		CameraRot = FRotator(-90.f, 0.f, 0.f);
		Pos = CameraLoc + Pos;
		GetController()->SetControlRotation(FRotator(0.f, 0.f, 0.f));
		break;
	case ESoldierCamreMode::E_CameraMode_WOW:
		CameraLoc = GetActorLocation();
		CameraRot = GetController()->GetControlRotation();
		Pos = CameraLoc - CameraRot.Vector()*m_CameraDistance + FVector(0.0f, 0.0f, 80.0f);
		break;
	default:
		GetActorEyesViewPoint(CameraLoc, CameraRot);
		Pos = CameraLoc;
		break;
	}
	//--------------------------------------------------------摄像机碰撞检测
	FCollisionQueryParams BoxParams(TEXT("Camera"), false, this);
	FHitResult Result;
	if (CameraLoc != Pos)
	{
		GetWorld()->SweepSingleByChannel(Result, CameraLoc, Pos, FQuat::Identity, ECC_Camera, FCollisionShape::MakeBox(FVector(12.f)), BoxParams);
	}

	//--------------------------------------------------------摄像机切换平滑处理
	m_SwitchBlendTimeToGo -= DeltaTime;
	if (m_SwitchBlendTimeToGo > 0.f)
	{
		float Dur = (m_CameraBlendTime - m_SwitchBlendTimeToGo) / m_CameraBlendTime;
		Pos = FMath::Lerp(m_SwitchBlendStartLoc, Pos, FMath::Pow(Dur, 1.f / m_CameraBlendExp));
		CameraRot = FMath::Lerp(m_SwitchBlendStartRot, CameraRot, FMath::Pow(Dur, 1.f / m_CameraBlendExp));
	}
	//--------------------------------------------------------摄像机缩放平滑处理
	m_ZoomBlendTimeToGo -= DeltaTime;
	if (m_ZoomBlendTimeToGo > 0.f)
	{
		float Dur = (m_CameraBlendTime - m_ZoomBlendTimeToGo) / m_CameraBlendTime;
		m_CameraDistance = FMath::Lerp(m_ZoomBlendStartDis, m_ZoomBlendEndDis, FMath::Pow(Dur, 1.f / m_CameraBlendExp));
	}

	OutResult.Location = (Result.GetActor() == NULL) ? Pos : Result.Location;

	OutResult.Rotation = CameraRot;
}

void ASoldierCharacter::UpdateMesh1PCameraTransform(const FVector& CameraLocation, const FRotator& CameraRotation)
{
	USkeletalMeshComponent* DefMesh1P = Cast<USkeletalMeshComponent>(GetClass()->GetDefaultSubobjectByName(TEXT("PawnMesh1P")));
	const FMatrix DefMeshLS = FRotationTranslationMatrix(DefMesh1P->GetRelativeRotation(), DefMesh1P->GetRelativeLocation());
	const FMatrix LocalToWorld = ActorToWorld().ToMatrixWithScale();

	// Mesh rotating code expect uniform scale in LocalToWorld matrix

	const FRotator RotCameraPitch(CameraRotation.Pitch, 0.0f, 0.0f);
	const FRotator RotCameraYaw(0.0f, CameraRotation.Yaw, 0.0f);

	const FMatrix LeveledCameraLS = FRotationTranslationMatrix(RotCameraYaw, CameraLocation) * LocalToWorld.Inverse();
	const FMatrix PitchedCameraLS = FRotationMatrix(RotCameraPitch) * LeveledCameraLS;
	const FMatrix MeshRelativeToCamera = DefMeshLS * LeveledCameraLS.Inverse();
	const FMatrix PitchedMesh = MeshRelativeToCamera * PitchedCameraLS;

	Mesh1P->SetRelativeLocationAndRotation(PitchedMesh.GetOrigin(), PitchedMesh.Rotator());
}


//////////////////////////////////////////////////////////////////////////
// Damage & death


void ASoldierCharacter::FellOutOfWorld(const class UDamageType& dmgType)
{
	Die(Health, FDamageEvent(dmgType.GetClass()), NULL, NULL);
}

void ASoldierCharacter::Suicide()
{
	KilledBy(this);
}

void ASoldierCharacter::KilledBy(APawn* EventInstigator)
{
	if (GetLocalRole() == ROLE_Authority && !bIsDying)
	{
		AController* Killer = NULL;
		if (EventInstigator != NULL)
		{
			Killer = EventInstigator->Controller;
			LastHitBy = NULL;
		}

		Die(Health, FDamageEvent(UDamageType::StaticClass()), Killer, NULL);
	}
}


float ASoldierCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	ASoldierPlayerController* MyPC = Cast<ASoldierPlayerController>(Controller);
	if (MyPC && MyPC->HasGodMode())
	{
		return 0.f;
	}

	if (Health <= 0.f)
	{
		return 0.f;
	}

	// Modify based on game rules.
	ASoldierGameMode* const Game = GetWorld()->GetAuthGameMode<ASoldierGameMode>();
	Damage = Game ? Game->ModifyDamage(Damage, this, DamageEvent, EventInstigator, DamageCauser) : 0.f;

	const float ActualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage > 0.f)
	{
		Health -= ActualDamage;
		if (Health <= 0)
		{
			Die(ActualDamage, DamageEvent, EventInstigator, DamageCauser);
		}
		else
		{
			PlayHit(ActualDamage, DamageEvent, EventInstigator ? EventInstigator->GetPawn() : NULL, DamageCauser);
		}

		MakeNoise(1.0f, EventInstigator ? EventInstigator->GetPawn() : this);
	}

	return ActualDamage;
}


bool ASoldierCharacter::CanDie(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser) const
{
	if (bIsDying										// already dying
		|| IsPendingKill()								// already destroyed
		|| GetLocalRole() != ROLE_Authority				// not authority
		|| GetWorld()->GetAuthGameMode<ASoldierGameMode>() == NULL
		|| GetWorld()->GetAuthGameMode<ASoldierGameMode>()->GetMatchState() == MatchState::LeavingMap)	// level transition occurring
	{
		return false;
	}

	return true;
}


bool ASoldierCharacter::Die(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser)
{
	if (!CanDie(KillingDamage, DamageEvent, Killer, DamageCauser))
	{
		return false;
	}

	Health = FMath::Min(0.0f, Health);

	// if this is an environmental death then refer to the previous killer so that they receive credit (knocked into lava pits, etc)
	UDamageType const* const DamageType = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();
	Killer = GetDamageInstigator(Killer, *DamageType);

	AController* const KilledPlayer = (Controller != NULL) ? Controller : Cast<AController>(GetOwner());
	GetWorld()->GetAuthGameMode<ASoldierGameMode>()->Killed(Killer, KilledPlayer, this, DamageType);

	NetUpdateFrequency = GetDefault<ASoldierCharacter>()->NetUpdateFrequency;
	GetCharacterMovement()->ForceReplicationUpdate();

	OnDeath(KillingDamage, DamageEvent, Killer ? Killer->GetPawn() : NULL, DamageCauser);
	return true;
}


void ASoldierCharacter::OnDeath(float KillingDamage, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser)
{
	if (bIsDying)
	{
		return;
	}

	SetReplicatingMovement(false);
	TearOff();
	bIsDying = true;

	if (GetLocalRole() == ROLE_Authority)
	{
		ReplicateHit(KillingDamage, DamageEvent, PawnInstigator, DamageCauser, true);

		// play the force feedback effect on the client player controller
		ASoldierPlayerController* PC = Cast<ASoldierPlayerController>(Controller);
		if (PC && DamageEvent.DamageTypeClass)
		{
			USoldierDamageType *DamageType = Cast<USoldierDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());
			if (DamageType && DamageType->KilledForceFeedback && PC->IsVibrationEnabled())
			{
				FForceFeedbackParameters FFParams;
				FFParams.Tag = "Damage";
				PC->ClientPlayForceFeedback(DamageType->KilledForceFeedback, FFParams);
			}
		}
	}

	// cannot use IsLocallyControlled here, because even local client's controller may be NULL here
	if (GetNetMode() != NM_DedicatedServer && DeathSound && Mesh1P && Mesh1P->IsVisible())
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	}

	// remove all weapons
	DestroyInventory();

	// switch back to 3rd person view
	UpdatePawnMeshes();

	DetachFromControllerPendingDestroy();
	StopAllAnimMontages();

	if (LowHealthWarningPlayer && LowHealthWarningPlayer->IsPlaying())
	{
		LowHealthWarningPlayer->Stop();
	}

	if (RunLoopAC)
	{
		RunLoopAC->Stop();
	}

	if (GetMesh())
	{
		static FName CollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetCollisionProfileName(CollisionProfileName);
	}
	SetActorEnableCollision(true);

	// Death anim
	float DeathAnimDuration = PlayAnimMontage(DeathAnim);

	// Ragdoll
	if (DeathAnimDuration > 0.f)
	{
		// Trigger ragdoll a little before the animation early so the character doesn't
		// blend back to its normal position.
		const float TriggerRagdollTime = DeathAnimDuration - 0.7f;

		// Enable blend physics so the bones are properly blending against the montage.
		GetMesh()->bBlendPhysics = true;

		// Use a local timer handle as we don't need to store it for later but we don't need to look for something to clear
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, this, &ASoldierCharacter::SetRagdollPhysics, FMath::Max(0.1f, TriggerRagdollTime), false);
	}
	else
	{
		SetRagdollPhysics();
	}

	// disable collisions on capsule
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
}

void ASoldierCharacter::PlayHit(float DamageTaken, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		ReplicateHit(DamageTaken, DamageEvent, PawnInstigator, DamageCauser, false);

		// play the force feedback effect on the client player controller
		ASoldierPlayerController* PC = Cast<ASoldierPlayerController>(Controller);
		if (PC && DamageEvent.DamageTypeClass)
		{
			USoldierDamageType *DamageType = Cast<USoldierDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());
			if (DamageType && DamageType->HitForceFeedback && PC->IsVibrationEnabled())
			{
				FForceFeedbackParameters FFParams;
				FFParams.Tag = "Damage";
				PC->ClientPlayForceFeedback(DamageType->HitForceFeedback, FFParams);
			}
		}
	}

	if (DamageTaken > 0.f)
	{
		ApplyDamageMomentum(DamageTaken, DamageEvent, PawnInstigator, DamageCauser);
	}

	ASoldierPlayerController* MyPC = Cast<ASoldierPlayerController>(Controller);
	ASoldierHUD* MyHUD = MyPC ? Cast<ASoldierHUD>(MyPC->GetHUD()) : NULL;
	if (MyHUD)
	{
		MyHUD->NotifyWeaponHit(DamageTaken, DamageEvent, PawnInstigator);
	}

	if (PawnInstigator && PawnInstigator != this && PawnInstigator->IsLocallyControlled())
	{
		ASoldierPlayerController* InstigatorPC = Cast<ASoldierPlayerController>(PawnInstigator->Controller);
		ASoldierHUD* InstigatorHUD = InstigatorPC ? Cast<ASoldierHUD>(InstigatorPC->GetHUD()) : NULL;
		if (InstigatorHUD)
		{
			InstigatorHUD->NotifyEnemyHit();
		}
	}
}


void ASoldierCharacter::SetRagdollPhysics()
{
	bool bInRagdoll = false;

	if (IsPendingKill())
	{
		bInRagdoll = false;
	}
	else if (!GetMesh() || !GetMesh()->GetPhysicsAsset())
	{
		bInRagdoll = false;
	}
	else
	{
		// initialize physics/etc
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->WakeAllRigidBodies();
		GetMesh()->bBlendPhysics = true;

		bInRagdoll = true;
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->SetComponentTickEnabled(false);

	if (!bInRagdoll)
	{
		// hide and set short lifespan
		TurnOff();
		SetActorHiddenInGame(true);
		SetLifeSpan(1.0f);
	}
	else
	{
		SetLifeSpan(10.0f);
	}
}



void ASoldierCharacter::ReplicateHit(float Damage, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser, bool bKilled)
{
	const float TimeoutTime = GetWorld()->GetTimeSeconds() + 0.5f;

	FDamageEvent const& LastDamageEvent = LastTakeHitInfo.GetDamageEvent();
	if ((PawnInstigator == LastTakeHitInfo.PawnInstigator.Get()) && (LastDamageEvent.DamageTypeClass == LastTakeHitInfo.DamageTypeClass) && (LastTakeHitTimeTimeout == TimeoutTime))
	{
		// same frame damage
		if (bKilled && LastTakeHitInfo.bKilled)
		{
			// Redundant death take hit, just ignore it
			return;
		}

		// otherwise, accumulate damage done this frame
		Damage += LastTakeHitInfo.ActualDamage;
	}

	LastTakeHitInfo.ActualDamage = Damage;
	LastTakeHitInfo.PawnInstigator = Cast<ASoldierCharacter>(PawnInstigator);
	LastTakeHitInfo.DamageCauser = DamageCauser;
	LastTakeHitInfo.SetDamageEvent(DamageEvent);
	LastTakeHitInfo.bKilled = bKilled;
	LastTakeHitInfo.EnsureReplication();

	LastTakeHitTimeTimeout = TimeoutTime;
}

void ASoldierCharacter::OnRep_LastTakeHitInfo()
{
	if (LastTakeHitInfo.bKilled)
	{
		OnDeath(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetDamageEvent(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get());
	}
	else
	{
		PlayHit(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetDamageEvent(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get());
	}
}

//Pawn::PlayDying sets this lifespan, but when that function is called on client, dead pawn's role is still SimulatedProxy despite bTearOff being true. 
void ASoldierCharacter::TornOff()
{
	SetLifeSpan(25.f);
}

bool ASoldierCharacter::IsMoving()
{
	return FMath::Abs(GetLastMovementInputVector().Size()) > 0.f;
}

//////////////////////////////////////////////////////////////////////////
// Inventory

void ASoldierCharacter::SpawnDefaultInventory()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		return;
	}

	int32 NumWeaponClasses = DefaultInventoryClasses.Num();
	for (int32 i = 0; i < NumWeaponClasses; i++)
	{
		if (DefaultInventoryClasses[i])
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ASoldierWeapon* NewWeapon = GetWorld()->SpawnActor<ASoldierWeapon>(DefaultInventoryClasses[i], SpawnInfo);
			AddWeapon(NewWeapon);
		}
	}

	// equip first weapon in inventory
	if (Inventory.Num() > 0)
	{
		EquipWeapon(Inventory[0]);
	}
}

void ASoldierCharacter::DestroyInventory()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		return;
	}

	// remove all weapons from inventory and destroy them
	for (int32 i = Inventory.Num() - 1; i >= 0; i--)
	{
		ASoldierWeapon* Weapon = Inventory[i];
		if (Weapon)
		{
			RemoveWeapon(Weapon);
			Weapon->Destroy();
		}
	}
}

void ASoldierCharacter::AddWeapon(ASoldierWeapon* Weapon)
{
	if (Weapon && GetLocalRole() == ROLE_Authority)
	{
		Weapon->OnEnterInventory(this);
		Inventory.AddUnique(Weapon);
	}
}

void ASoldierCharacter::RemoveWeapon(ASoldierWeapon* Weapon)
{
	if (Weapon && GetLocalRole() == ROLE_Authority)
	{
		Weapon->OnLeaveInventory();
		Inventory.RemoveSingle(Weapon);
	}
}

ASoldierWeapon* ASoldierCharacter::FindWeapon(TSubclassOf<ASoldierWeapon> WeaponClass)
{
	for (int32 i = 0; i < Inventory.Num(); i++)
	{
		if (Inventory[i] && Inventory[i]->IsA(WeaponClass))
		{
			return Inventory[i];
		}
	}

	return NULL;
}

void ASoldierCharacter::EquipWeapon(ASoldierWeapon* Weapon)
{
	if (Weapon)
	{
		if (GetLocalRole() == ROLE_Authority)
		{
			SetCurrentWeapon(Weapon, CurrentWeapon);
		}
		else
		{
			ServerEquipWeapon(Weapon);
		}
		GetWeapon()->SwapMesh1P3PWeaponHidden();
	}
}

bool ASoldierCharacter::ServerEquipWeapon_Validate(ASoldierWeapon* Weapon)
{
	return true;
}

void ASoldierCharacter::ServerEquipWeapon_Implementation(ASoldierWeapon* Weapon)
{
	EquipWeapon(Weapon);
}

void ASoldierCharacter::OnRep_CurrentWeapon(ASoldierWeapon* LastWeapon)
{
	SetCurrentWeapon(CurrentWeapon, LastWeapon);
}

void ASoldierCharacter::SetCurrentWeapon(ASoldierWeapon* NewWeapon, ASoldierWeapon* LastWeapon)
{
	ASoldierWeapon* LocalLastWeapon = nullptr;

	if (LastWeapon != NULL)
	{
		LocalLastWeapon = LastWeapon;
	}
	else if (NewWeapon != CurrentWeapon)
	{
		LocalLastWeapon = CurrentWeapon;
	}

	// unequip previous
	if (LocalLastWeapon)
	{
		LocalLastWeapon->OnUnEquip();
	}

	CurrentWeapon = NewWeapon;

	// equip new one
	if (NewWeapon)
	{
		NewWeapon->SetOwningPawn(this);	// Make sure weapon's MyPawn is pointing back to us. During replication, we can't guarantee APawn::CurrentWeapon will rep after AWeapon::MyPawn!

		NewWeapon->OnEquip(LastWeapon);
	}
}


//////////////////////////////////////////////////////////////////////////
// Weapon usage

void ASoldierCharacter::StartWeaponFire()
{
	if (!bWantsToFire)
	{
		bWantsToFire = true;
		if (CurrentWeapon)
		{
			CurrentWeapon->StartFire();
		}
	}
}

void ASoldierCharacter::StopWeaponFire()
{
	if (bWantsToFire)
	{
		bWantsToFire = false;
		if (CurrentWeapon)
		{
			CurrentWeapon->StopFire();
		}
	}
}

bool ASoldierCharacter::CanFire() const
{
	return IsAlive();
}

bool ASoldierCharacter::CanReload() const
{
	return true;
}

void ASoldierCharacter::SetTargeting(bool bNewTargeting)
{
	bIsTargeting = bNewTargeting;

	if (TargetingSound)
	{
		UGameplayStatics::SpawnSoundAttached(TargetingSound, GetRootComponent());
	}

	if (GetLocalRole() < ROLE_Authority)
	{
		ServerSetTargeting(bNewTargeting);
	}
}

bool ASoldierCharacter::ServerSetTargeting_Validate(bool bNewTargeting)
{
	return true;
}

void ASoldierCharacter::ServerSetTargeting_Implementation(bool bNewTargeting)
{
	SetTargeting(bNewTargeting);
}

//////////////////////////////////////////////////////////////////////////
// Movement

void ASoldierCharacter::SetRunning(bool bNewRunning, bool bToggle)
{
	bWantsToRun = bNewRunning;
	bWantsToRunToggled = bNewRunning && bToggle;

	if (GetLocalRole() < ROLE_Authority)
	{
		ServerSetRunning(bNewRunning, bToggle);
	}
}

bool ASoldierCharacter::ServerSetRunning_Validate(bool bNewRunning, bool bToggle)
{
	return true;
}

void ASoldierCharacter::ServerSetRunning_Implementation(bool bNewRunning, bool bToggle)
{
	SetRunning(bNewRunning, bToggle);
}

void ASoldierCharacter::UpdateRunSounds()
{
	const bool bIsRunSoundPlaying = RunLoopAC != nullptr && RunLoopAC->IsActive();
	const bool bWantsRunSoundPlaying = IsRunning() && IsMoving();

	// Don't bother playing the sounds unless we're running and moving.
	if (!bIsRunSoundPlaying && bWantsRunSoundPlaying)
	{
		if (RunLoopAC != nullptr)
		{
			RunLoopAC->Play();
		}
		else if (RunLoopSound != nullptr)
		{
			RunLoopAC = UGameplayStatics::SpawnSoundAttached(RunLoopSound, GetRootComponent());
			if (RunLoopAC != nullptr)
			{
				RunLoopAC->bAutoDestroy = false;
			}
		}
	}
	else if (bIsRunSoundPlaying && !bWantsRunSoundPlaying)
	{
		RunLoopAC->Stop();
		if (RunStopSound != nullptr)
		{
			UGameplayStatics::SpawnSoundAttached(RunStopSound, GetRootComponent());
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Animations

float ASoldierCharacter::PlayAnimMontage(class UAnimMontage* AnimMontage, float InPlayRate, FName StartSectionName)
{
	USkeletalMeshComponent* UseMesh = GetPawnMesh();
	if (AnimMontage && UseMesh && UseMesh->AnimScriptInstance)
	{
		return UseMesh->AnimScriptInstance->Montage_Play(AnimMontage, InPlayRate);
	}

	return 0.0f;
}

void ASoldierCharacter::StopAnimMontage(class UAnimMontage* AnimMontage)
{
	USkeletalMeshComponent* UseMesh = GetPawnMesh();
	if (AnimMontage && UseMesh && UseMesh->AnimScriptInstance &&
		UseMesh->AnimScriptInstance->Montage_IsPlaying(AnimMontage))
	{
		UseMesh->AnimScriptInstance->Montage_Stop(AnimMontage->BlendOut.GetBlendTime(), AnimMontage);
	}
}

void ASoldierCharacter::StopAllAnimMontages()
{
	USkeletalMeshComponent* UseMesh = GetPawnMesh();
	if (UseMesh && UseMesh->AnimScriptInstance)
	{
		UseMesh->AnimScriptInstance->Montage_Stop(0.0f);
	}
}


//////////////////////////////////////////////////////////////////////////
// Input

void ASoldierCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);
	PlayerInputComponent->BindAxis("MoveForward", this, &ASoldierCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASoldierCharacter::MoveRight);
	PlayerInputComponent->BindAxis("MoveUp", this, &ASoldierCharacter::MoveUp);
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ASoldierCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ASoldierCharacter::LookUpAtRate);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ASoldierCharacter::OnStartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ASoldierCharacter::OnStopFire);

	PlayerInputComponent->BindAction("Targeting", IE_Pressed, this, &ASoldierCharacter::OnStartTargeting);
	PlayerInputComponent->BindAction("Targeting", IE_Released, this, &ASoldierCharacter::OnStopTargeting);

	PlayerInputComponent->BindAction("NextWeapon", IE_Pressed, this, &ASoldierCharacter::OnNextWeapon);
	PlayerInputComponent->BindAction("PrevWeapon", IE_Pressed, this, &ASoldierCharacter::OnPrevWeapon);

	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ASoldierCharacter::OnReload);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASoldierCharacter::OnStartJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ASoldierCharacter::OnStopJump);

	PlayerInputComponent->BindAction("Run", IE_Pressed, this, &ASoldierCharacter::OnStartRunning);
	PlayerInputComponent->BindAction("RunToggle", IE_Pressed, this, &ASoldierCharacter::OnStartRunningToggle);
	PlayerInputComponent->BindAction("Run", IE_Released, this, &ASoldierCharacter::OnStopRunning);

	//First Person Or ThirdPerson to be Changed
	PlayerInputComponent->BindAction("CameraMode", IE_Released, this, &ASoldierCharacter::ChangeCameraMode);

	PlayerInputComponent->BindAction("SoldierCrouch", IE_Released, this, &ASoldierCharacter::SoldierAnimCrouch);

	PlayerInputComponent->BindAction("SoldierProne", IE_Released, this, &ASoldierCharacter::SoldierAnimProne);
}


void ASoldierCharacter::MoveForward(float Val)
{
	if (Controller && Val != 0.f)
	{
		// Limit pitch when walking or falling
		const bool bLimitRotation =  (GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling());
		const FRotator Rotation =( bLimitRotation) ? Controller->GetControlRotation() : GetActorRotation() ;

		//GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3600.0f, FColor(255, 48, 16), FString::SanitizeFloat(Rotation.Yaw));
		const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis(EAxis::X);
		AddMovementInput(Direction, Val);
	}
}

void ASoldierCharacter::MoveRight(float Val)
{
	if (Val != 0.f)
	{
		FRotator Rotation;
		if (m_ECameraMode == E_CameraMode_WOW)
		{
			if (m_bViewingCharacter)
				Rotation = GetActorRotation();
			else
				Rotation = GetControlRotation();
		}
		else
			Rotation = GetControlRotation();

		FRotator YawRotation(0, Rotation.Yaw, 0);

		// Get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		//const FQuat Rotation = GetActorQuat();
		//const FVector Direction = FQuatRotationMatrix(Rotation).GetScaledAxis(EAxis::Y);
		AddMovementInput(Direction, Val);
	}
}

void ASoldierCharacter::MoveUp(float Val)
{
	if (Val != 0.f)
	{
		// Not when walking or falling.
		if (GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling())
		{
			return;
		}

		AddMovementInput(FVector::UpVector, Val);
	}
}

void ASoldierCharacter::TurnAtRate(float Val)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Val * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ASoldierCharacter::LookUpAtRate(float Val)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Val * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ASoldierCharacter::OnStartFire()
{
	ASoldierPlayerController* MyPC = Cast<ASoldierPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (IsRunning())
		{
			SetRunning(false, false);
		}
		StartWeaponFire();
	}
}

void ASoldierCharacter::OnStopFire()
{
	StopWeaponFire();
}

void ASoldierCharacter::OnStartTargeting()
{
	ASoldierPlayerController* MyPC = Cast<ASoldierPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (IsRunning())
		{
			SetRunning(false, false);
		}
		SetTargeting(true);
	}
}

void ASoldierCharacter::OnStopTargeting()
{
	SetTargeting(false);
}

void ASoldierCharacter::OnNextWeapon()
{
	ASoldierPlayerController* MyPC = Cast<ASoldierPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (Inventory.Num() >= 2 && (CurrentWeapon == NULL || CurrentWeapon->GetCurrentState() != EWeaponState::Equipping))
		{
			const int32 CurrentWeaponIdx = Inventory.IndexOfByKey(CurrentWeapon);
			ASoldierWeapon* NextWeapon = Inventory[(CurrentWeaponIdx + 1) % Inventory.Num()];
			EquipWeapon(NextWeapon);
		}
	}
}

void ASoldierCharacter::OnPrevWeapon()
{
	ASoldierPlayerController* MyPC = Cast<ASoldierPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (Inventory.Num() >= 2 && (CurrentWeapon == NULL || CurrentWeapon->GetCurrentState() != EWeaponState::Equipping))
		{
			const int32 CurrentWeaponIdx = Inventory.IndexOfByKey(CurrentWeapon);
			ASoldierWeapon* PrevWeapon = Inventory[(CurrentWeaponIdx - 1 + Inventory.Num()) % Inventory.Num()];
			EquipWeapon(PrevWeapon);
		}
	}
}

void ASoldierCharacter::OnReload()
{
	ASoldierPlayerController* MyPC = Cast<ASoldierPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (CurrentWeapon)
		{
			CurrentWeapon->StartReload();
		}
	}
}

void ASoldierCharacter::OnStartRunning()
{
	ASoldierPlayerController* MyPC = Cast<ASoldierPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (IsTargeting())
		{
			SetTargeting(false);
		}
		StopWeaponFire();
		SetRunning(true, false);
	}
}

void ASoldierCharacter::OnStartRunningToggle()
{
	ASoldierPlayerController* MyPC = Cast<ASoldierPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (IsTargeting())
		{
			SetTargeting(false);
		}
		StopWeaponFire();
		SetRunning(true, true);
	}
}

void ASoldierCharacter::OnStopRunning()
{
	SetRunning(false, false);
}

bool ASoldierCharacter::IsRunning() const
{
	if (!GetCharacterMovement())
	{
		return false;
	}

	return (bWantsToRun || bWantsToRunToggled) && !GetVelocity().IsZero() && (GetVelocity().GetSafeNormal2D() | GetActorForwardVector()) > -0.1;
}


void ASoldierCharacter::ChangeCameraMode() {

	m_SwitchBlendTimeToGo = m_CameraBlendTime;
	m_SwitchBlendStartLoc = Cast<ASoldierPlayerController>(GetController())->PlayerCameraManager->GetCameraLocation();
	m_SwitchBlendStartRot = Cast<ASoldierPlayerController>(GetController())->PlayerCameraManager->GetCameraRotation();
	FString strMode = "empty ";
	m_ECameraMode = (enum ESoldierCamreMode)(m_ECameraMode + 1);
	if (m_ECameraMode >= ESoldierCamreMode::E_CameraMode_Max)
	{
		m_ECameraMode = (enum ESoldierCamreMode) 1;
	}

	switch (m_ECameraMode)
	{
	case ESoldierCamreMode::E_CameraMode_First:
	{
		//移动方向为当前Controller方向
		GetCharacterMovement()->bOrientRotationToMovement = false;
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 1000.f, 0.0f);
		bUseControllerRotationPitch = false;
		bUseControllerRotationYaw = true;
		bUseControllerRotationRoll = false;
		m_bMesh3P = false;
		//第一人称视角下，角色模型自己看不见，其他人能看见
		GetMesh()->SetOwnerNoSee(true);
		GetMesh()->SetOnlyOwnerSee(false);
		//第一人称视角下，手臂只有自己能看见
		Mesh1P->SetOwnerNoSee(false);
		Mesh1P->SetOnlyOwnerSee(true);
		Mesh1P->SetHiddenInGame(false);

		strMode = "E_CameraMode_First";
		break;
	}
	case ESoldierCamreMode::E_CameraMode_Third:
	{
		//移动方向为当前Controller方向
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

		bUseControllerRotationPitch = false;
		bUseControllerRotationYaw = false;
		bUseControllerRotationRoll = false;
		m_bMesh3P = true;
		//第三人称视角下，角色模型自己可以看见，其他人也可以看见
		GetMesh()->SetOwnerNoSee(false);
		GetMesh()->SetOnlyOwnerSee(false);
		//第三人称视角下，直接隐藏手臂
		Mesh1P->SetHiddenInGame(true);
		Mesh1P->SetOwnerNoSee(true);
		Mesh1P->SetOnlyOwnerSee(true);

		strMode = "E_CameraMode_Third";
		break;
	}
	case ESoldierCamreMode::E_CameraMode_ThirdRot:
	{//移动方向为当前Controller方向
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

		bUseControllerRotationPitch = false;
		bUseControllerRotationYaw = true;
		bUseControllerRotationRoll = false;
		m_bMesh3P = true;
		//第三人称视角下，角色模型自己可以看见，其他人也可以看见
		GetMesh()->SetOwnerNoSee(false);
		GetMesh()->SetOnlyOwnerSee(false);
		//第三人称视角下，直接隐藏手臂
		Mesh1P->SetHiddenInGame(true);
		Mesh1P->SetOwnerNoSee(true);
		Mesh1P->SetOnlyOwnerSee(true);

		strMode = "E_CameraMode_ThirdRot";
		break;
	}
	case ESoldierCamreMode::E_CameraMode_Top:
	{
		//移动方向为当前Controller方向
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

		bUseControllerRotationPitch = false;
		bUseControllerRotationYaw = false;
		bUseControllerRotationRoll = false;
		m_bMesh3P = true;
		//第三人称视角下，角色模型自己可以看见，其他人也可以看见
		GetMesh()->SetOwnerNoSee(false);
		GetMesh()->SetOnlyOwnerSee(false);
		//第三人称视角下，直接隐藏手臂
		Mesh1P->SetHiddenInGame(true);
		Mesh1P->SetOwnerNoSee(true);
		Mesh1P->SetOnlyOwnerSee(true);

		strMode = "E_CameraMode_Top";

		break;
	}
	case ESoldierCamreMode::E_CameraMode_WOW:
	{
		//移动方向为当前Controller方向
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.f, 0.0f);

		bUseControllerRotationPitch = false;
		bUseControllerRotationYaw = false;
		bUseControllerRotationRoll = false;
		m_bMesh3P = true;
		//第三人称视角下，角色模型自己可以看见，其他人也可以看见
		GetMesh()->SetOwnerNoSee(false);
		GetMesh()->SetOnlyOwnerSee(false);
		//第三人称视角下，直接隐藏手臂
		Mesh1P->SetHiddenInGame(true);
		Mesh1P->SetOwnerNoSee(true);
		Mesh1P->SetOnlyOwnerSee(true);

		strMode = "E_CameraMode_WOW";
		break;
	}
	default:
		break;
	}
	if(GetWeapon())
		GetWeapon()->SwapMesh1P3PWeaponHidden();
	GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Red, strMode);
}


/** Player Run Crouch Animation */
void ASoldierCharacter::SoldierAnimCrouch() {
	bWantsToCrouch = !bWantsToCrouch;
	//bWantsToCrouch = true;
	bWantsToProne = false;
	bIsCrouched = bWantsToCrouch;
	float maxWalkSpeed;
	bWantsToCrouch == 1 ? maxWalkSpeed = 220.f : maxWalkSpeed = 375.0f;
	GetCharacterMovement()->MaxWalkSpeed = maxWalkSpeed;
	//GetCharacterMovement()->MaxWalkSpeedCrouched = maxWalkSpeed;
}

/** Player Run Prone Animation */
void ASoldierCharacter::SoldierAnimProne(){
	bWantsToCrouch = false;
	bWantsToProne = !bWantsToProne;

	float maxProneSpeed;
	bWantsToProne == 1 ? maxProneSpeed = 28.0f : maxProneSpeed = 375.0f;
	GetCharacterMovement()->MaxWalkSpeed = maxProneSpeed;
}

void ASoldierCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bWantsToRunToggled && !IsRunning())
	{
		SetRunning(false, false);
	}
	ASoldierPlayerController* MyPC = Cast<ASoldierPlayerController>(Controller);
	if (MyPC && MyPC->HasHealthRegen())
	{
		if (this->Health < this->GetMaxHealth())
		{
			this->Health += 5 * DeltaSeconds;
			if (Health > this->GetMaxHealth())
			{
				Health = this->GetMaxHealth();
			}
		}
	}

	if (GEngine->UseSound())
	{
		if (LowHealthSound)
		{
			if ((this->Health > 0 && this->Health < this->GetMaxHealth() * LowHealthPercentage) && (!LowHealthWarningPlayer || !LowHealthWarningPlayer->IsPlaying()))
			{
				LowHealthWarningPlayer = UGameplayStatics::SpawnSoundAttached(LowHealthSound, GetRootComponent(),
					NAME_None, FVector(ForceInit), EAttachLocation::KeepRelativeOffset, true);
				if (LowHealthWarningPlayer)
				{
					LowHealthWarningPlayer->SetVolumeMultiplier(0.0f);
				}
			}
			else if ((this->Health > this->GetMaxHealth() * LowHealthPercentage || this->Health < 0) && LowHealthWarningPlayer && LowHealthWarningPlayer->IsPlaying())
			{
				LowHealthWarningPlayer->Stop();
			}
			if (LowHealthWarningPlayer && LowHealthWarningPlayer->IsPlaying())
			{
				const float MinVolume = 0.3f;
				const float VolumeMultiplier = (1.0f - (this->Health / (this->GetMaxHealth() * LowHealthPercentage)));
				LowHealthWarningPlayer->SetVolumeMultiplier(MinVolume + (1.0f - MinVolume) * VolumeMultiplier);
			}
		}

		UpdateRunSounds();
	}

	const APlayerController* PC = Cast<APlayerController>(GetController());
	const bool bLocallyControlled = (PC ? PC->IsLocalController() : false);
	const uint32 UniqueID = GetUniqueID();
	FAudioThread::RunCommandOnAudioThread([UniqueID, bLocallyControlled]()
	{
	    USoundNodeLocalPlayer::GetLocallyControlledActorCache().Add(UniqueID, bLocallyControlled);
	});
	
	TArray<FVector> PointsToTest;
	BuildPauseReplicationCheckPoints(PointsToTest);

	if (NetVisualizeRelevancyTestPoints == 1)
	{
		for (FVector PointToTest : PointsToTest)
		{
			DrawDebugSphere(GetWorld(), PointToTest, 10.0f, 8, FColor::Red);
		}
	}
}

void ASoldierCharacter::BeginDestroy()
{
	Super::BeginDestroy();

	if (!GExitPurge)
	{
		const uint32 UniqueID = GetUniqueID();
		FAudioThread::RunCommandOnAudioThread([UniqueID]()
		{
			USoundNodeLocalPlayer::GetLocallyControlledActorCache().Remove(UniqueID);
		});
	}
}

void ASoldierCharacter::OnStartJump()
{
	ASoldierPlayerController* MyPC = Cast<ASoldierPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		bPressedJump = true;
	}
}

void ASoldierCharacter::OnStopJump()
{
	bPressedJump = false;
	StopJumping();
}

//////////////////////////////////////////////////////////////////////////
// Replication

void ASoldierCharacter::PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	// Only replicate this property for a short duration after it changes so join in progress players don't get spammed with fx when joining late
	DOREPLIFETIME_ACTIVE_OVERRIDE(ASoldierCharacter, LastTakeHitInfo, GetWorld() && GetWorld()->GetTimeSeconds() < LastTakeHitTimeTimeout);
}

void ASoldierCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// only to local owner: weapon change requests are locally instigated, other clients don't need it
	DOREPLIFETIME_CONDITION(ASoldierCharacter, Inventory, COND_OwnerOnly);

	// everyone except local owner: flag change is locally instigated
	DOREPLIFETIME_CONDITION(ASoldierCharacter, bIsTargeting, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ASoldierCharacter, bWantsToRun, COND_SkipOwner);

	DOREPLIFETIME_CONDITION(ASoldierCharacter, LastTakeHitInfo, COND_Custom);

	// everyone
	DOREPLIFETIME(ASoldierCharacter, CurrentWeapon);
	DOREPLIFETIME(ASoldierCharacter, Health);
}

bool ASoldierCharacter::IsReplicationPausedForConnection(const FNetViewer& ConnectionOwnerNetViewer)
{
	if (NetEnablePauseRelevancy == 1)
	{
		APlayerController* PC = Cast<APlayerController>(ConnectionOwnerNetViewer.InViewer);
		check(PC);

		FVector ViewLocation;
		FRotator ViewRotation;
		PC->GetPlayerViewPoint(ViewLocation, ViewRotation);

		FCollisionQueryParams CollisionParams(SCENE_QUERY_STAT(LineOfSight), true, PC->GetPawn());
		CollisionParams.AddIgnoredActor(this);

		TArray<FVector> PointsToTest;
		BuildPauseReplicationCheckPoints(PointsToTest);

		for (FVector PointToTest : PointsToTest)
		{
			if (!GetWorld()->LineTraceTestByChannel(PointToTest, ViewLocation, ECC_Visibility, CollisionParams))
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

void ASoldierCharacter::OnReplicationPausedChanged(bool bIsReplicationPaused)
{
	GetMesh()->SetHiddenInGame(bIsReplicationPaused, true);
}

ASoldierWeapon* ASoldierCharacter::GetWeapon() const
{
	return CurrentWeapon;
}

int32 ASoldierCharacter::GetInventoryCount() const
{
	return Inventory.Num();
}

ASoldierWeapon* ASoldierCharacter::GetInventoryWeapon(int32 index) const
{
	return Inventory[index];
}

USkeletalMeshComponent* ASoldierCharacter::GetPawnMesh() const
{
	return IsFirstPerson() ? Mesh1P : GetMesh();
}

USkeletalMeshComponent* ASoldierCharacter::GetSpecifcPawnMesh(bool WantFirstPerson) const
{
	return WantFirstPerson == true ? Mesh1P : GetMesh();
}

FName ASoldierCharacter::GetWeaponAttachPoint() const
{
	return WeaponAttachPoint;
}

float ASoldierCharacter::GetTargetingSpeedModifier() const
{
	return TargetingSpeedModifier;
}

bool ASoldierCharacter::IsTargeting() const
{
	return bIsTargeting;
}

float ASoldierCharacter::GetRunningSpeedModifier() const
{
	return RunningSpeedModifier;
}

bool ASoldierCharacter::IsFiring() const
{
	return bWantsToFire;
};


bool ASoldierCharacter::IsCrouched() const
{
	return bWantsToCrouch;
}


bool ASoldierCharacter::IsProning() const
{
	return bWantsToProne;
}

bool ASoldierCharacter::IsFirstPerson() const
{
	return IsAlive() && Controller && Controller->IsLocalPlayerController() && IsMesh3P();
}

bool ASoldierCharacter::IsMesh3P() const {
	return !m_bMesh3P;
}
int32 ASoldierCharacter::GetMaxHealth() const
{
	return GetClass()->GetDefaultObject<ASoldierCharacter>()->Health;
}

bool ASoldierCharacter::IsAlive() const
{
	return Health > 0;
}

float ASoldierCharacter::GetLowHealthPercentage() const
{
	return LowHealthPercentage;
}

void ASoldierCharacter::UpdateTeamColorsAllMIDs()
{
	for (int32 i = 0; i < MeshMIDs.Num(); ++i)
	{
		UpdateTeamColors(MeshMIDs[i]);
	}
}

void ASoldierCharacter::BuildPauseReplicationCheckPoints(TArray<FVector>& RelevancyCheckPoints)
{
	FBoxSphereBounds Bounds = GetCapsuleComponent()->CalcBounds(GetCapsuleComponent()->GetComponentTransform());
	FBox BoundingBox = Bounds.GetBox();
	float XDiff = Bounds.BoxExtent.X * 2;
	float YDiff = Bounds.BoxExtent.Y * 2;

	RelevancyCheckPoints.Add(BoundingBox.Min);
	RelevancyCheckPoints.Add(FVector(BoundingBox.Min.X + XDiff, BoundingBox.Min.Y, BoundingBox.Min.Z));
	RelevancyCheckPoints.Add(FVector(BoundingBox.Min.X, BoundingBox.Min.Y + YDiff, BoundingBox.Min.Z));
	RelevancyCheckPoints.Add(FVector(BoundingBox.Min.X + XDiff, BoundingBox.Min.Y + YDiff, BoundingBox.Min.Z));
	RelevancyCheckPoints.Add(FVector(BoundingBox.Max.X - XDiff, BoundingBox.Max.Y, BoundingBox.Max.Z));
	RelevancyCheckPoints.Add(FVector(BoundingBox.Max.X, BoundingBox.Max.Y - YDiff, BoundingBox.Max.Z));
	RelevancyCheckPoints.Add(FVector(BoundingBox.Max.X - XDiff, BoundingBox.Max.Y - YDiff, BoundingBox.Max.Z));
	RelevancyCheckPoints.Add(BoundingBox.Max);
}
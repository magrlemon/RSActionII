// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SoldierGame.h"
#include "Pickups/SoldierPickup.h"
#include "Particles/ParticleSystemComponent.h"

ASoldierPickup::ASoldierPickup(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	UCapsuleComponent* CollisionComp = ObjectInitializer.CreateDefaultSubobject<UCapsuleComponent>(this, TEXT("CollisionComp"));
	CollisionComp->InitCapsuleSize(40.0f, 50.0f);
	CollisionComp->SetCollisionObjectType(COLLISION_PICKUP);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RootComponent = CollisionComp;

	PickupPSC = ObjectInitializer.CreateDefaultSubobject<UParticleSystemComponent>(this, TEXT("PickupFX"));
	PickupPSC->bAutoActivate = false;
	PickupPSC->bAutoDestroy = false;
	PickupPSC->SetupAttachment(RootComponent);

	RespawnTime = 10.0f;
	bIsActive = false;
	PickedUpBy = NULL;

	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
}

void ASoldierPickup::BeginPlay()
{
	Super::BeginPlay();

	RespawnPickup();

	// register on pickup list (server only), don't care about unregistering (in FinishDestroy) - no streaming
	ASoldierGameMode* GameMode = GetWorld()->GetAuthGameMode<ASoldierGameMode>();
	if (GameMode)
	{
		GameMode->LevelPickups.Add(this);
	}
}

void ASoldierPickup::NotifyActorBeginOverlap(class AActor* Other)
{
	Super::NotifyActorBeginOverlap(Other);
	PickupOnTouch(Cast<ASoldierCharacter>(Other));
}

bool ASoldierPickup::CanBePickedUp(class ASoldierCharacter* TestPawn) const
{
	return TestPawn && TestPawn->IsAlive();
}

void ASoldierPickup::GivePickupTo(class ASoldierCharacter* Pawn)
{
}

void ASoldierPickup::PickupOnTouch(class ASoldierCharacter* Pawn)
{
	if (bIsActive && Pawn && Pawn->IsAlive() && !IsPendingKill())
	{
		if (CanBePickedUp(Pawn))
		{
			GivePickupTo(Pawn);
			PickedUpBy = Pawn;

			if (!IsPendingKill())
			{
				bIsActive = false;
				OnPickedUp();

				if (RespawnTime > 0.0f)
				{
					GetWorldTimerManager().SetTimer(TimerHandle_RespawnPickup, this, &ASoldierPickup::RespawnPickup, RespawnTime, false);
				}
			}
		}
	}
}

void ASoldierPickup::RespawnPickup()
{
	bIsActive = true;
	PickedUpBy = NULL;
	OnRespawned();

	TSet<AActor*> OverlappingPawns;
	GetOverlappingActors(OverlappingPawns, ASoldierCharacter::StaticClass());

	for (AActor* OverlappingPawn : OverlappingPawns)
	{
		PickupOnTouch(CastChecked<ASoldierCharacter>(OverlappingPawn));	
	}
}

void ASoldierPickup::OnPickedUp()
{
	if (RespawningFX)
	{
		PickupPSC->SetTemplate(RespawningFX);
		PickupPSC->ActivateSystem();
	}
	else
	{
		PickupPSC->DeactivateSystem();
	}

	if (PickupSound && PickedUpBy)
	{
		UGameplayStatics::SpawnSoundAttached(PickupSound, PickedUpBy->GetRootComponent());
	}

	OnPickedUpEvent();
}

void ASoldierPickup::OnRespawned()
{
	if (ActiveFX)
	{
		PickupPSC->SetTemplate(ActiveFX);
		PickupPSC->ActivateSystem();
	}
	else
	{
		PickupPSC->DeactivateSystem();
	}

	const bool bJustSpawned = CreationTime <= (GetWorld()->GetTimeSeconds() + 5.0f);
	if (RespawnSound && !bJustSpawned)
	{
		UGameplayStatics::PlaySoundAtLocation(this, RespawnSound, GetActorLocation());
	}

	OnRespawnEvent();
}

void ASoldierPickup::OnRep_IsActive()
{
	if (bIsActive)
	{
		OnRespawned();
	}
	else
	{
		OnPickedUp();
	}
}

void ASoldierPickup::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( ASoldierPickup, bIsActive );
	DOREPLIFETIME( ASoldierPickup, PickedUpBy );
}
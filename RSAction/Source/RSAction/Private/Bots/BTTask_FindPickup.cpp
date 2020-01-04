// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
#include "Bots/BTTask_FindPickup.h"
#include "SoldierGame.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyAllTypes.h"
#include "Bots/SoldierAIController.h"
#include "Bots/SoldierBot.h"
#include "Pickups/SoldierPickup_Ammo.h"
#include "Weapons/SoldierWeapon_Instant.h"

UBTTask_FindPickup::UBTTask_FindPickup(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer)
{
}

EBTNodeResult::Type UBTTask_FindPickup::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ASoldierAIController* MyController = Cast<ASoldierAIController>(OwnerComp.GetAIOwner());
	ASoldierBot* MyBot = MyController ? Cast<ASoldierBot>(MyController->GetPawn()) : NULL;
	if (MyBot == NULL)
	{
		return EBTNodeResult::Failed;
	}

	ASoldierGameMode* GameMode = MyBot->GetWorld()->GetAuthGameMode<ASoldierGameMode>();
	if (GameMode == NULL)
	{
		return EBTNodeResult::Failed;
	}

	const FVector MyLoc = MyBot->GetActorLocation();
	ASoldierPickup_Ammo* BestPickup = NULL;
	float BestDistSq = MAX_FLT;

	for (int32 i = 0; i < GameMode->LevelPickups.Num(); ++i)
	{
		ASoldierPickup_Ammo* AmmoPickup = Cast<ASoldierPickup_Ammo>(GameMode->LevelPickups[i]);
		if (AmmoPickup && AmmoPickup->IsForWeapon(ASoldierWeapon_Instant::StaticClass()) && AmmoPickup->CanBePickedUp(MyBot))
		{
			const float DistSq = (AmmoPickup->GetActorLocation() - MyLoc).SizeSquared();
			if (BestDistSq == -1 || DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestPickup = AmmoPickup;
			}
		}
	}

	if (BestPickup)
	{
		OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID(), BestPickup->GetActorLocation());
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}

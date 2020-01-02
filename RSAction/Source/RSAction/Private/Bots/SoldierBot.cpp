// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
#include "Bots/SoldierBot.h"
#include "RSAction.h"

#include "Bots/SoldierAIController.h"

ASoldierBot::ASoldierBot(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer)
{
	AIControllerClass = ASoldierAIController::StaticClass();

	UpdatePawnMeshes();

	bUseControllerRotationYaw = true;
}

bool ASoldierBot::IsFirstPerson() const
{
	return false;
}

void ASoldierBot::FaceRotation(FRotator NewRotation, float DeltaTime)
{
	FRotator CurrentRotation = FMath::RInterpTo(GetActorRotation(), NewRotation, DeltaTime, 8.0f);

	Super::FaceRotation(CurrentRotation, DeltaTime);
}

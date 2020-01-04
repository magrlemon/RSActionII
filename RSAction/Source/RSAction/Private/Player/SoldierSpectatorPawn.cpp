// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.


#include "SoldierGame.h"
#include "Player/SoldierSpectatorPawn.h"

ASoldierSpectatorPawn::ASoldierSpectatorPawn(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bReplicates = false;
}

void ASoldierSpectatorPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);
	
	PlayerInputComponent->BindAxis("MoveForward", this, &ADefaultPawn::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ADefaultPawn::MoveRight);
	PlayerInputComponent->BindAxis("MoveUp", this, &ADefaultPawn::MoveUp_World);
	PlayerInputComponent->BindAxis("Turn", this, &ADefaultPawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ADefaultPawn::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &ADefaultPawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ASoldierSpectatorPawn::LookUpAtRate);
}

void ASoldierSpectatorPawn::LookUpAtRate(float Val)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Val * BaseLookUpRate * GetWorld()->GetDeltaSeconds() * CustomTimeDilation);
}

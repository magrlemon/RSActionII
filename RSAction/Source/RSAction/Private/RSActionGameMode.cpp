// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "RSActionGameMode.h"
#include "RSActionCharacter.h"
#include "UObject/ConstructorHelpers.h"

ARSActionGameMode::ARSActionGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/Soldier_Character_01"));//Soldier_Character_01.Soldier_Character_01
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

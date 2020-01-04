// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Engine.h"
#include "SlateBasics.h"
#include "SlateExtras.h"
#include "ParticleDefinitions.h"
#include "SoundDefinitions.h"
#include "Net/UnrealNetwork.h"
#include "Online/SoldierGameMode.h"
#include "Online/SoldierGameState.h"
#include "RSActionCharacter.h"
#include "Player/SoldierCharacterMovement.h"
#include "Player/SoldierPlayerController.h"
#include "Player/SoldierCharacter.h"
#include "Player/SoldierLocalPlayer.h"
//#include "RSActionGameClasses.h"


class UBehaviorTreeComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogSoldier, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogSoldierWeapon, Log, All);

/** when you modify this, please note that this information can be saved with instances
 * also DefaultEngine.ini [/Script/Engine.CollisionProfile] should match with this list **/
#define COLLISION_WEAPON		ECC_GameTraceChannel1
#define COLLISION_PROJECTILE	ECC_GameTraceChannel2
#define COLLISION_PICKUP		ECC_GameTraceChannel3

#define MAX_PLAYER_NAME_LENGTH 16


#ifndef SOLDIER_CONSOLE_UI
 /** Set to 1 to pretend we're building for console even on a PC, for testing purposes */
#define SOLDIER_SIMULATE_CONSOLE_UI	0

#if PLATFORM_PS4 || PLATFORM_XBOXONE || PLATFORM_SWITCH || SOLDIER_SIMULATE_CONSOLE_UI || PLATFORM_QUAIL
#define SOLDIER_CONSOLE_UI 1
#else
#define SOLDIER_CONSOLE_UI 0
#endif
#endif

// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SoldierEngine.cpp: SoldierEngine c++ code.
=============================================================================*/

#include "RSAction.h"
#include "SoldierEngine.h"
#include "SoldierGameInstance.h"

USoldierEngine::USoldierEngine(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USoldierEngine::Init(IEngineLoop* InEngineLoop)
{
	// Note: Lots of important things happen in Super::Init(), including spawning the player pawn in-game and
	// creating the renderer.
	Super::Init(InEngineLoop);
}


void USoldierEngine::HandleNetworkFailure(UWorld *World, UNetDriver *NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	// Determine if we need to change the King state based on network failures.

	// Only handle failure at this level for game or pending net drivers.
	FName NetDriverName = NetDriver ? NetDriver->NetDriverName : NAME_None; 
	if (NetDriverName == NAME_GameNetDriver || NetDriverName == NAME_PendingNetDriver)
	{
		// If this net driver has already been unregistered with this world, then don't handle it.
		//if (World)
		{
			//UNetDriver * NetDriver = FindNamedNetDriver(World, NetDriverName);
			if (NetDriver)
			{
				switch (FailureType)
				{
					case ENetworkFailure::FailureReceived:
					{
						USoldierGameInstance* const SoldierInstance = Cast<USoldierGameInstance>(GameInstance);
						if (SoldierInstance && NetDriver->GetNetMode() == NM_Client)
						{
							const FText OKButton = NSLOCTEXT( "DialogButtons", "OKAY", "OK" );

							// NOTE - We pass in false here to not override the message if we are already going to the main menu
							// We're going to make the assumption that someone else has a better message than "Lost connection to host" if
							// this is the case
							SoldierInstance->ShowMessageThenGotoState( FText::FromString(ErrorString), OKButton, FText::GetEmpty(), SoldierGameInstanceState::MainMenu, false );
						}
						break;
					}
					case ENetworkFailure::PendingConnectionFailure:						
					{
						USoldierGameInstance* const GI = Cast<USoldierGameInstance>(GameInstance);
						if (GI && NetDriver->GetNetMode() == NM_Client)
						{
							const FText OKButton = NSLOCTEXT( "DialogButtons", "OKAY", "OK" );

							// NOTE - We pass in false here to not override the message if we are already going to the main menu
							// We're going to make the assumption that someone else has a better message than "Lost connection to host" if
							// this is the case
							GI->ShowMessageThenGotoState( FText::FromString(ErrorString), OKButton, FText::GetEmpty(), SoldierGameInstanceState::MainMenu, false );
						}
						break;
					}
					case ENetworkFailure::ConnectionLost:						
					case ENetworkFailure::ConnectionTimeout:
					{
						USoldierGameInstance* const GI = Cast<USoldierGameInstance>(GameInstance);
						if (GI && NetDriver->GetNetMode() == NM_Client)
						{
							const FText ReturnReason	= NSLOCTEXT( "NetworkErrors", "HostDisconnect", "Lost connection to host." );
							const FText OKButton		= NSLOCTEXT( "DialogButtons", "OKAY", "OK" );

							// NOTE - We pass in false here to not override the message if we are already going to the main menu
							// We're going to make the assumption that someone else has a better message than "Lost connection to host" if
							// this is the case
							GI->ShowMessageThenGotoState( ReturnReason, OKButton, FText::GetEmpty(), SoldierGameInstanceState::MainMenu, false );
						}
						break;
					}
					case ENetworkFailure::NetDriverAlreadyExists:
					case ENetworkFailure::NetDriverCreateFailure:
					case ENetworkFailure::OutdatedClient:
					case ENetworkFailure::OutdatedServer:
					default:
						break;
				}
			}
		}
	}

	// standard failure handling.
	Super::HandleNetworkFailure(World, NetDriver, FailureType, ErrorString);
}


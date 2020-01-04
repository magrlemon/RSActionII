// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SoldierGame.h"
#include "Player/SoldierDemoSpectator.h"
#include "UI/Menu/SoldierDemoPlaybackMenu.h"
#include "UI/Widgets/SSoldierDemoHUD.h"
#include "Engine/DemoNetDriver.h"

ASoldierDemoSpectator::ASoldierDemoSpectator(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bShowMouseCursor = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	bShouldPerformFullTickWhenPaused = true;
}

void ASoldierDemoSpectator::SetupInputComponent()
{
	Super::SetupInputComponent();

	// UI input
	InputComponent->BindAction( "InGameMenu", IE_Pressed, this, &ASoldierDemoSpectator::OnToggleInGameMenu );

	InputComponent->BindAction( "NextWeapon", IE_Pressed, this, &ASoldierDemoSpectator::OnIncreasePlaybackSpeed );
	InputComponent->BindAction( "PrevWeapon", IE_Pressed, this, &ASoldierDemoSpectator::OnDecreasePlaybackSpeed );
}

void ASoldierDemoSpectator::SetPlayer( UPlayer* InPlayer )
{
	Super::SetPlayer( InPlayer );

	// Build menu only after game is initialized
	SoldierDemoPlaybackMenu = MakeShareable( new FSoldierDemoPlaybackMenu() );
	SoldierDemoPlaybackMenu->Construct( Cast< ULocalPlayer >( Player ) );

	// Create HUD if this is playback
	if (GetWorld() != nullptr && GetWorld()->DemoNetDriver != nullptr && !GetWorld()->DemoNetDriver->IsServer())
	{
		if (GEngine != nullptr && GEngine->GameViewport != nullptr)
		{
			DemoHUD = SNew(SSoldierDemoHUD)
				.PlayerOwner(this);

			GEngine->GameViewport->AddViewportWidgetContent(DemoHUD.ToSharedRef());
		}
	}

	FActorSpawnParameters SpawnInfo;

	SpawnInfo.Owner				= this;
	SpawnInfo.Instigator		= GetInstigator();
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	PlaybackSpeed = 2;

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(DemoHUD);

	SetInputMode(InputMode);
}

void ASoldierDemoSpectator::OnToggleInGameMenu()
{
	// if no one's paused, pause
	if ( SoldierDemoPlaybackMenu.IsValid() )
	{
		SoldierDemoPlaybackMenu->ToggleGameMenu();
	}
}

static float PlaybackSpeedLUT[5] = { 0.1f, 0.5f, 1.0f, 2.0f, 4.0f };

void ASoldierDemoSpectator::OnIncreasePlaybackSpeed()
{
	PlaybackSpeed = FMath::Clamp( PlaybackSpeed + 1, 0, 4 );

	GetWorldSettings()->DemoPlayTimeDilation = PlaybackSpeedLUT[ PlaybackSpeed ];
}

void ASoldierDemoSpectator::OnDecreasePlaybackSpeed()
{
	PlaybackSpeed = FMath::Clamp( PlaybackSpeed - 1, 0, 4 );

	GetWorldSettings()->DemoPlayTimeDilation = PlaybackSpeedLUT[ PlaybackSpeed ];
}

void ASoldierDemoSpectator::Destroyed()
{
	if (GEngine != nullptr && GEngine->GameViewport != nullptr && DemoHUD.IsValid())
	{
		// Remove HUD
		GEngine->GameViewport->RemoveViewportWidgetContent(DemoHUD.ToSharedRef());
	}

	Super::Destroyed();
}

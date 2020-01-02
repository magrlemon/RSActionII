// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SoldierDemoSpectator.generated.h"

class SSoldierDemoHUD;

UCLASS(config=Game)
class ASoldierDemoSpectator : public APlayerController
{
	GENERATED_UCLASS_BODY()

public:
	/** shooter in-game menu */
	TSharedPtr<class FSoldierDemoPlaybackMenu> SoldierDemoPlaybackMenu;

	virtual void SetupInputComponent() override;
	virtual void SetPlayer( UPlayer* Player ) override;
	virtual void Destroyed() override;

	void OnToggleInGameMenu();
	void OnIncreasePlaybackSpeed();
	void OnDecreasePlaybackSpeed();

	int32 PlaybackSpeed;

private:
	TSharedPtr<SSoldierDemoHUD> DemoHUD;
};


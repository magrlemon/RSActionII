// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SoldierGame.h"
#include "SoldierGameUserSettings.h"

USoldierGameUserSettings::USoldierGameUserSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetToDefaults();
}

void USoldierGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();

	GraphicsQuality = 1;	
	bIsLanMatch = true;
	bIsDedicatedServer = false;
}

void USoldierGameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	if (GraphicsQuality == 0)
	{
		ScalabilityQuality.SetFromSingleQualityLevel(1);
	}
	else
	{
		ScalabilityQuality.SetFromSingleQualityLevel(3);
	}

	Super::ApplySettings(bCheckForCommandLineOverrides);
}
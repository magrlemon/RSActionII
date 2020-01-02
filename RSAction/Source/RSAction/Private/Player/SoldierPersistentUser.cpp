// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "RSAction.h"
#include "Player/SoldierPersistentUser.h"
#include "SoldierLocalPlayer.h"

USoldierPersistentUser::USoldierPersistentUser(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetToDefaults();
}

void USoldierPersistentUser::SetToDefaults()
{
	bIsDirty = false;

	bVibrationOpt = true;
	bInvertedYAxis = false;
	AimSensitivity = 1.0f;
	Gamma = 2.2f;
	BotsCount = 1;
	bIsRecordingDemos = false;
}

bool USoldierPersistentUser::IsAimSensitivityDirty() const
{
	bool bIsAimSensitivityDirty = false;

	// Fixme: USoldierPersistentUser is not setup to work with multiple worlds.
	// For now, user settings are global to all world instances.
	if (GEngine)
	{
		TArray<APlayerController*> PlayerList;
		GEngine->GetAllLocalPlayerControllers(PlayerList);

		for (auto It = PlayerList.CreateIterator(); It; ++It)
		{
			APlayerController* PC = *It;
			if (!PC || !PC->Player || !PC->PlayerInput)
			{
				continue;
			}

			// Update key bindings for the current user only
			USoldierLocalPlayer* LocalPlayer = Cast<USoldierLocalPlayer>(PC->Player);
			if(!LocalPlayer || LocalPlayer->GetPersistentUser() != this)
			{
				continue;
			}

			// check if the aim sensitivity is off anywhere
			for (int32 Idx = 0; Idx < PC->PlayerInput->AxisMappings.Num(); Idx++)
			{
				FInputAxisKeyMapping &AxisMapping = PC->PlayerInput->AxisMappings[Idx];
				if (AxisMapping.AxisName == "Lookup" || AxisMapping.AxisName == "LookupRate" || AxisMapping.AxisName == "Turn" || AxisMapping.AxisName == "TurnRate")
				{
					if (FMath::Abs(AxisMapping.Scale) != GetAimSensitivity())
					{
						bIsAimSensitivityDirty = true;
						break;
					}
				}
			}
		}
	}

	return bIsAimSensitivityDirty;
}

bool USoldierPersistentUser::IsInvertedYAxisDirty() const
{
	bool bIsInvertedYAxisDirty = false;
	if (GEngine)
	{
		TArray<APlayerController*> PlayerList;
		GEngine->GetAllLocalPlayerControllers(PlayerList);

		for (auto It = PlayerList.CreateIterator(); It; ++It)
		{
			APlayerController* PC = *It;
			if (!PC || !PC->Player || !PC->PlayerInput)
			{
				continue;
			}

			// Update key bindings for the current user only
			USoldierLocalPlayer* LocalPlayer = Cast<USoldierLocalPlayer>(PC->Player);
			if(!LocalPlayer || LocalPlayer->GetPersistentUser() != this)
			{
				continue;
			}

			bIsInvertedYAxisDirty |= PC->PlayerInput->GetInvertAxis("Lookup") != GetInvertedYAxis();
			bIsInvertedYAxisDirty |= PC->PlayerInput->GetInvertAxis("LookupRate") != GetInvertedYAxis();
		}
	}

	return bIsInvertedYAxisDirty;
}

void USoldierPersistentUser::SavePersistentUser()
{
	UGameplayStatics::SaveGameToSlot(this, SlotName, UserIndex);
	bIsDirty = false;
}

USoldierPersistentUser* USoldierPersistentUser::LoadPersistentUser(FString SlotName, const int32 UserIndex)
{
	USoldierPersistentUser* Result = nullptr;
	
	// first set of player signins can happen before the UWorld exists, which means no OSS, which means no user names, which means no slotnames.
	// Persistent users aren't valid in this state.
	if (SlotName.Len() > 0)
	{
		if (!GIsBuildMachine && UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
		{
			Result = Cast<USoldierPersistentUser>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
		}

		if (Result == nullptr)
		{
			// if failed to load, create a new one
			Result = Cast<USoldierPersistentUser>( UGameplayStatics::CreateSaveGameObject(USoldierPersistentUser::StaticClass()) );
		}
		check(Result != nullptr);
	
		Result->SlotName = SlotName;
		Result->UserIndex = UserIndex;
	}

	return Result;
}

void USoldierPersistentUser::SaveIfDirty()
{
	if (bIsDirty || IsInvertedYAxisDirty() || IsAimSensitivityDirty())
	{
		SavePersistentUser();
	}
}

void USoldierPersistentUser::AddMatchResult(int32 MatchKills, int32 MatchDeaths, int32 MatchBulletsFired, int32 MatchRocketsFired, bool bIsMatchWinner)
{
	Kills += MatchKills;
	Deaths += MatchDeaths;
	BulletsFired += MatchBulletsFired;
	RocketsFired += MatchRocketsFired;
	
	if (bIsMatchWinner)
	{
		Wins++;
	}
	else
	{
		Losses++;
	}

	bIsDirty = true;
}

void USoldierPersistentUser::TellInputAboutKeybindings()
{
	TArray<APlayerController*> PlayerList;
	GEngine->GetAllLocalPlayerControllers(PlayerList);

	for (auto It = PlayerList.CreateIterator(); It; ++It)
	{
		APlayerController* PC = *It;
		if (!PC || !PC->Player || !PC->PlayerInput)
		{
			continue;
		}

		// Update key bindings for the current user only
		USoldierLocalPlayer* LocalPlayer = Cast<USoldierLocalPlayer>(PC->Player);
		if(!LocalPlayer || LocalPlayer->GetPersistentUser() != this)
		{
			continue;
		}

		//set the aim sensitivity
		for (int32 Idx = 0; Idx < PC->PlayerInput->AxisMappings.Num(); Idx++)
		{
			FInputAxisKeyMapping &AxisMapping = PC->PlayerInput->AxisMappings[Idx];
			if (AxisMapping.AxisName == "Lookup" || AxisMapping.AxisName == "LookupRate" || AxisMapping.AxisName == "Turn" || AxisMapping.AxisName == "TurnRate")
			{
				AxisMapping.Scale = (AxisMapping.Scale < 0.0f) ? -GetAimSensitivity() : +GetAimSensitivity();
			}
		}
		PC->PlayerInput->ForceRebuildingKeyMaps();

		//invert it, and if does not equal our bool, invert it again
		if (PC->PlayerInput->GetInvertAxis("LookupRate") != GetInvertedYAxis())
		{
			PC->PlayerInput->InvertAxis("LookupRate");
		}

		if (PC->PlayerInput->GetInvertAxis("Lookup") != GetInvertedYAxis())
		{
			PC->PlayerInput->InvertAxis("Lookup");
		}
	}
}

int32 USoldierPersistentUser::GetUserIndex() const
{
	return UserIndex;
}

void USoldierPersistentUser::SetVibration(bool bVibration)
{
	bIsDirty |= bVibrationOpt != bVibration;

	bVibrationOpt = bVibration;

}

void USoldierPersistentUser::SetInvertedYAxis(bool bInvert)
{
	bIsDirty |= bInvertedYAxis != bInvert;

	bInvertedYAxis = bInvert;
}

void USoldierPersistentUser::SetAimSensitivity(float InSensitivity)
{
	bIsDirty |= AimSensitivity != InSensitivity;

	AimSensitivity = InSensitivity;
}

void USoldierPersistentUser::SetGamma(float InGamma)
{
	bIsDirty |= Gamma != InGamma;

	Gamma = InGamma;
}

void USoldierPersistentUser::SetBotsCount(int32 InCount)
{
	bIsDirty |= BotsCount != InCount;

	BotsCount = InCount;
}

void USoldierPersistentUser::SetIsRecordingDemos(const bool InbIsRecordingDemos)
{
	bIsDirty |= bIsRecordingDemos != InbIsRecordingDemos;

	bIsRecordingDemos = InbIsRecordingDemos;
}

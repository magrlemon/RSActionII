// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "RSAction.h"
#include "SoldierOptions.h"
#include "SoldierTypes.h"
#include "SoldierStyle.h"
#include "SoldierOptionsWidgetStyle.h"
#include "ShooterGameUserSettings.h"
#include "Player/SoldierPersistentUser.h"
#include "Player/SoldierLocalPlayer.h"

#define LOCTEXT_NAMESPACE "RSAction.HUD.Menu"

void FSoldierOptions::Construct(ULocalPlayer* InPlayerOwner)
{
	OptionsStyle = &FSoldierStyle::Get().GetWidgetStyle<FSoldierOptionsStyle>("DefaultSoldierOptionsStyle");

	PlayerOwner = InPlayerOwner;
	MinSensitivity = 1;
	
	TArray<FText> ResolutionList;
	TArray<FText> OnOffList;
	TArray<FText> SensitivityList;
	TArray<FText> GammaList;
	TArray<FText> LowHighList;

	FDisplayMetrics DisplayMetrics;
	FSlateApplication::Get().GetInitialDisplayMetrics(DisplayMetrics);

	bool bAddedNativeResolution = false;
	const FIntPoint NativeResolution(DisplayMetrics.PrimaryDisplayWidth, DisplayMetrics.PrimaryDisplayHeight);

	for (int32 i = 0; i < DefaultSoldierResCount; i++)
	{
		if (DefaultSoldierResolutions[i].X <= DisplayMetrics.PrimaryDisplayWidth && DefaultSoldierResolutions[i].Y <= DisplayMetrics.PrimaryDisplayHeight)
		{
			ResolutionList.Add(FText::Format(FText::FromString("{0}x{1}"), FText::FromString(FString::FromInt(DefaultSoldierResolutions[i].X)), FText::FromString(FString::FromInt(DefaultSoldierResolutions[i].Y))));
			Resolutions.Add(DefaultSoldierResolutions[i]);

			bAddedNativeResolution = bAddedNativeResolution || (DefaultSoldierResolutions[i] == NativeResolution);
		}
	}

	// Always make sure that the native resolution is available
	if (!bAddedNativeResolution)
	{
		ResolutionList.Add(FText::Format(FText::FromString("{0}x{1}"), FText::FromString(FString::FromInt(NativeResolution.X)), FText::FromString(FString::FromInt(NativeResolution.Y))));
		Resolutions.Add(NativeResolution);
	}

	OnOffList.Add(LOCTEXT("Off","OFF"));
	OnOffList.Add(LOCTEXT("On","ON"));

	LowHighList.Add(LOCTEXT("Low","LOW"));
	LowHighList.Add(LOCTEXT("High","HIGH"));

	//Mouse sensitivity 0-50
	for (int32 i = 0; i < 51; i++)
	{
		SensitivityList.Add(FText::AsNumber(i));
	}

	for (int32 i = -50; i < 51; i++)
	{
		GammaList.Add(FText::AsNumber(i));
	}

	/** Options menu root item */
	TSharedPtr<FSoldierMenuItem> OptionsRoot = FSoldierMenuItem::CreateRoot();

	/** Cheats menu root item */
	TSharedPtr<FSoldierMenuItem> CheatsRoot = FSoldierMenuItem::CreateRoot();

	CheatsItem = MenuHelper::AddMenuItem(CheatsRoot,LOCTEXT("Cheats", "CHEATS"));
	MenuHelper::AddMenuOptionSP(CheatsItem, LOCTEXT("InfiniteAmmo", "INFINITE AMMO"), OnOffList, this, &FSoldierOptions::InfiniteAmmoOptionChanged);
	MenuHelper::AddMenuOptionSP(CheatsItem, LOCTEXT("InfiniteClip", "INFINITE CLIP"), OnOffList, this, &FSoldierOptions::InfiniteClipOptionChanged);
	MenuHelper::AddMenuOptionSP(CheatsItem, LOCTEXT("FreezeMatchTimer", "FREEZE MATCH TIMER"), OnOffList, this, &FSoldierOptions::FreezeTimerOptionChanged);
	MenuHelper::AddMenuOptionSP(CheatsItem, LOCTEXT("HealthRegen", "HP REGENERATION"), OnOffList, this, &FSoldierOptions::HealthRegenOptionChanged);

	OptionsItem = MenuHelper::AddMenuItem(OptionsRoot,LOCTEXT("Options", "OPTIONS"));
#if PLATFORM_DESKTOP
	VideoResolutionOption = MenuHelper::AddMenuOptionSP(OptionsItem,LOCTEXT("Resolution", "RESOLUTION"), ResolutionList, this, &FSoldierOptions::VideoResolutionOptionChanged);
	GraphicsQualityOption = MenuHelper::AddMenuOptionSP(OptionsItem,LOCTEXT("Quality", "QUALITY"),LowHighList, this, &FSoldierOptions::GraphicsQualityOptionChanged);
	FullScreenOption = MenuHelper::AddMenuOptionSP(OptionsItem,LOCTEXT("FullScreen", "FULL SCREEN"),OnOffList, this, &FSoldierOptions::FullScreenOptionChanged);
#endif
	GammaOption = MenuHelper::AddMenuOptionSP(OptionsItem,LOCTEXT("Gamma", "GAMMA CORRECTION"),GammaList, this, &FSoldierOptions::GammaOptionChanged);
	AimSensitivityOption = MenuHelper::AddMenuOptionSP(OptionsItem,LOCTEXT("AimSensitivity", "AIM SENSITIVITY"),SensitivityList, this, &FSoldierOptions::AimSensitivityOptionChanged);
	InvertYAxisOption = MenuHelper::AddMenuOptionSP(OptionsItem,LOCTEXT("InvertYAxis", "INVERT Y AXIS"),OnOffList, this, &FSoldierOptions::InvertYAxisOptionChanged);
	VibrationOption = MenuHelper::AddMenuOptionSP(OptionsItem, LOCTEXT("Vibration", "VIBRATION"), OnOffList, this, &FSoldierOptions::ToggleVibration);
	
	MenuHelper::AddMenuItemSP(OptionsItem,LOCTEXT("ApplyChanges", "APPLY CHANGES"), this, &FSoldierOptions::OnApplySettings);

	//Do not allow to set aim sensitivity to 0
	AimSensitivityOption->MinMultiChoiceIndex = MinSensitivity;
    
    //Default vibration to On.
	VibrationOption->SelectedMultiChoice = 1;

	UserSettings = CastChecked<UShooterGameUserSettings>(GEngine->GetGameUserSettings());
	ResolutionOpt = UserSettings->GetScreenResolution();
	bFullScreenOpt = UserSettings->GetFullscreenMode();
	GraphicsQualityOpt = UserSettings->GetGraphicsQuality();

	USoldierPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
		bInvertYAxisOpt = PersistentUser->GetInvertedYAxis();
		SensitivityOpt = PersistentUser->GetAimSensitivity();
		GammaOpt = PersistentUser->GetGamma();
		bVibrationOpt = PersistentUser->GetVibration();
	}
	else
	{
		bVibrationOpt = true;
		bInvertYAxisOpt = false;
		SensitivityOpt = 1.0f;
		GammaOpt = 2.2f;
	}

	if (ensure(PlayerOwner != nullptr))
	{
		APlayerController* BaseController = Cast<APlayerController>(UGameplayStatics::GetPlayerController(PlayerOwner->GetWorld(), GetOwnerUserIndex()));
		ASoldierPlayerController* SoldierPlayerController = Cast<ASoldierPlayerController>(UGameplayStatics::GetPlayerController(PlayerOwner->GetWorld(), GetOwnerUserIndex()));
		ensure(BaseController);
		if (BaseController)
		{
			if (SoldierPlayerController)
			{
				SoldierPlayerController->SetIsVibrationEnabled(bVibrationOpt);
			}
			else
			{
				// We are in the menus and therefore don't need to do anything as the controller is different
				// and can't store the vibration setting.
			}
		}
	}
}

void FSoldierOptions::OnApplySettings()
{
	FSlateApplication::Get().PlaySound(OptionsStyle->AcceptChangesSound, GetOwnerUserIndex());
	ApplySettings();
}

void FSoldierOptions::ApplySettings()
{
	USoldierPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
		PersistentUser->SetAimSensitivity(SensitivityOpt);
		PersistentUser->SetInvertedYAxis(bInvertYAxisOpt);
		PersistentUser->SetGamma(GammaOpt);
		PersistentUser->SetVibration(bVibrationOpt);
		PersistentUser->TellInputAboutKeybindings();

		PersistentUser->SaveIfDirty();
	}

	UserSettings->SetScreenResolution(ResolutionOpt);
	UserSettings->SetFullscreenMode(bFullScreenOpt);
	UserSettings->SetGraphicsQuality(GraphicsQualityOpt);
	UserSettings->ApplySettings(false);

	OnApplyChanges.ExecuteIfBound();
}

void FSoldierOptions::TellInputAboutKeybindings()
{
	USoldierPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
		PersistentUser->TellInputAboutKeybindings();
	}
}

void FSoldierOptions::RevertChanges()
{
	FSlateApplication::Get().PlaySound(OptionsStyle->DiscardChangesSound, GetOwnerUserIndex());
	UpdateOptions();
	GEngine->DisplayGamma =  2.2f + 2.0f * (-0.5f + GammaOption->SelectedMultiChoice / 100.0f);
}

int32 FSoldierOptions::GetCurrentResolutionIndex(FIntPoint CurrentRes)
{
	int32 Result = 0; // return first valid resolution if match not found
	for (int32 i = 0; i < Resolutions.Num(); i++)
	{
		if (Resolutions[i] == CurrentRes)
		{
			Result = i;
			break;
		}
	}
	return Result;
}

int32 FSoldierOptions::GetCurrentMouseYAxisInvertedIndex()
{
	USoldierPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
		return InvertYAxisOption->SelectedMultiChoice = PersistentUser->GetInvertedYAxis() ? 1 : 0;
	}
	else
	{
		return 0;
	}
}

int32 FSoldierOptions::GetCurrentMouseSensitivityIndex()
{
	USoldierPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
		//mouse sensitivity is a floating point value ranged from 0.0f to 1.0f
		int32 IntSensitivity = FMath::RoundToInt((PersistentUser->GetAimSensitivity() - 0.5f) * 10.0f);
		//Clamp to valid index range
		return FMath::Clamp(IntSensitivity, MinSensitivity, 100);
	}

	return FMath::RoundToInt((1.0f - 0.5f) * 10.0f);
}

int32 FSoldierOptions::GetCurrentGammaIndex()
{
	USoldierPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
		//reverse gamma calculation
		int32 GammaIndex = FMath::TruncToInt(((PersistentUser->GetGamma() - 2.2f) / 2.0f + 0.5f) * 100);
		//Clamp to valid index range
		return FMath::Clamp(GammaIndex, 0, 100);
	}

	return FMath::TruncToInt(((2.2f - 2.2f) / 2.0f + 0.5f) * 100);
}

int32 FSoldierOptions::GetOwnerUserIndex() const
{
	return PlayerOwner ? PlayerOwner->GetControllerId() : 0;
}

USoldierPersistentUser* FSoldierOptions::GetPersistentUser() const
{
	USoldierLocalPlayer* const SLP = Cast<USoldierLocalPlayer>(PlayerOwner);
	if (SLP)
	{
		return SLP->GetPersistentUser();
	}

	return nullptr;
}

void FSoldierOptions::UpdateOptions()
{
#if UE_BUILD_SHIPPING
	CheatsItem->bVisible = false;
#else
	//Toggle Cheat menu visibility depending if we are client or server
	UWorld* const World = PlayerOwner->GetWorld();
	if (World && World->GetNetMode() == NM_Client)
	{
		CheatsItem->bVisible = false;
	}
	else
	{
		CheatsItem->bVisible = true;
	}
#endif

	//grab the user settings
	USoldierPersistentUser* const PersistentUser = GetPersistentUser();
	if (PersistentUser)
	{
		// Update bInvertYAxisOpt, SensitivityOpt and GammaOpt because the SoldierOptions can be created without the controller having a player
		// by the in-game menu which will leave them with default values
		bInvertYAxisOpt = PersistentUser->GetInvertedYAxis();
		SensitivityOpt = PersistentUser->GetAimSensitivity();
		GammaOpt = PersistentUser->GetGamma();
		bVibrationOpt = PersistentUser->GetVibration();
	} 

	InvertYAxisOption->SelectedMultiChoice =  GetCurrentMouseYAxisInvertedIndex();
	AimSensitivityOption->SelectedMultiChoice = GetCurrentMouseSensitivityIndex();
	GammaOption->SelectedMultiChoice = GetCurrentGammaIndex();
	VibrationOption->SelectedMultiChoice = bVibrationOpt ? 1 : 0;

	GammaOptionChanged(GammaOption, GammaOption->SelectedMultiChoice);
#if PLATFORM_DESKTOP
	VideoResolutionOption->SelectedMultiChoice = GetCurrentResolutionIndex(UserSettings->GetScreenResolution());
	GraphicsQualityOption->SelectedMultiChoice = UserSettings->GetGraphicsQuality();
	FullScreenOption->SelectedMultiChoice = UserSettings->GetFullscreenMode() != EWindowMode::Windowed ? 1 : 0;
#endif
}

void FSoldierOptions::InfiniteAmmoOptionChanged(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex)
{
	UWorld* const World = PlayerOwner->GetWorld();
	if (World)
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			ASoldierPlayerController* SoldierPC = Cast<ASoldierPlayerController>(*It);
			if (SoldierPC)
			{
				SoldierPC->SetInfiniteAmmo(MultiOptionIndex > 0 ? true : false);
			}
		}
	}
}

void FSoldierOptions::InfiniteClipOptionChanged(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex)
{
	UWorld* const World = PlayerOwner->GetWorld();
	if (World)
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			ASoldierPlayerController* const SoldierPC = Cast<ASoldierPlayerController>(*It);
			if (SoldierPC)
			{
				SoldierPC->SetInfiniteClip(MultiOptionIndex > 0 ? true : false);
			}
		}
	}
}

void FSoldierOptions::FreezeTimerOptionChanged(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex)
{
	UWorld* const World = PlayerOwner->GetWorld();
	ASoldierGameState* const GameState = World ? World->GetGameState<ASoldierGameState>() : nullptr;
	if (GameState)
	{
		GameState->bTimerPaused = MultiOptionIndex > 0  ? true : false;
	}
}


void FSoldierOptions::HealthRegenOptionChanged(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex)
{
	UWorld* const World = PlayerOwner->GetWorld();
	if (World)
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			ASoldierPlayerController* const SoldierPC = Cast<ASoldierPlayerController>(*It);
			if (SoldierPC)
			{
				SoldierPC->SetHealthRegen(MultiOptionIndex > 0 ? true : false);
			}
		}
	}
}

void FSoldierOptions::VideoResolutionOptionChanged(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex)
{
	ResolutionOpt = Resolutions[MultiOptionIndex];
}

void FSoldierOptions::GraphicsQualityOptionChanged(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex)
{
	GraphicsQualityOpt = MultiOptionIndex;
}

void FSoldierOptions::FullScreenOptionChanged(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex)
{
	static auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.FullScreenMode"));
	EWindowMode::Type FullScreenMode = CVar->GetValueOnGameThread() == 1 ? EWindowMode::WindowedFullscreen : EWindowMode::Fullscreen;
	bFullScreenOpt = MultiOptionIndex == 0 ? EWindowMode::Windowed : FullScreenMode;
}

void FSoldierOptions::AimSensitivityOptionChanged(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex)
{
	SensitivityOpt = 0.5f + (MultiOptionIndex / 10.0f);
}

void FSoldierOptions::GammaOptionChanged(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex)
{
	GammaOpt = 2.2f + 2.0f * (-0.5f + MultiOptionIndex / 100.0f);
	GEngine->DisplayGamma = GammaOpt;
}

void FSoldierOptions::ToggleVibration(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex)
{
	bVibrationOpt = MultiOptionIndex > 0 ? true : false;
	APlayerController* BaseController = Cast<APlayerController>(UGameplayStatics::GetPlayerController(PlayerOwner->GetWorld(), GetOwnerUserIndex()));
	ASoldierPlayerController* SoldierPlayerController = Cast<ASoldierPlayerController>(UGameplayStatics::GetPlayerController(PlayerOwner->GetWorld(), GetOwnerUserIndex()));
	ensure(BaseController);
    if(BaseController)
    {
		if (SoldierPlayerController)
		{
			SoldierPlayerController->SetIsVibrationEnabled(bVibrationOpt);
		}
		else
		{
			// We are in the menus and therefore don't need to do anything as the controller is different
			// and can't store the vibration setting.
		}
    }
}

void FSoldierOptions::InvertYAxisOptionChanged(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex)
{
	bInvertYAxisOpt = MultiOptionIndex > 0  ? true : false;
}

#undef LOCTEXT_NAMESPACE

// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "SlateBasics.h"
#include "SlateExtras.h"
#include "Widgets/SoldierMenuItem.h"
#include "Widgets/SSoldierMenuWidget.h"

/** supported resolutions */
const FIntPoint DefaultSoldierResolutions[] = { FIntPoint(1024,768), FIntPoint(1280,720), FIntPoint(1920,1080) };

/** supported resolutions count*/
const int32 DefaultSoldierResCount = UE_ARRAY_COUNT(DefaultSoldierResolutions);

/** delegate called when changes are applied */
DECLARE_DELEGATE(FOnApplyChanges);

class URSActionGameUserSettings;

class FSoldierOptions : public TSharedFromThis<FSoldierOptions>
{
public:
	/** sets owning player controller */
	void Construct(ULocalPlayer* InPlayerOwner);

	/** get current options values for display */
	void UpdateOptions();

	/** UI callback for applying settings, plays sound */
	void OnApplySettings();

	/** applies changes in game settings */
	void ApplySettings();

	/** needed because we can recreate the subsystem that stores it */
	void TellInputAboutKeybindings();

	/** reverts non-saved changes in game settings */
	void RevertChanges();

	/** holds options menu item */
	TSharedPtr<FSoldierMenuItem> OptionsItem;

	/** holds cheats menu item */
	TSharedPtr<FSoldierMenuItem> CheatsItem;

	/** called when changes were applied - can be used to close submenu */
	FOnApplyChanges OnApplyChanges;

protected:
	/** User settings pointer */
	URSActionGameUserSettings* UserSettings;

	/** video resolution option changed handler */
	void VideoResolutionOptionChanged(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex);

	/** graphics quality option changed handler */
	void GraphicsQualityOptionChanged(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex);

	/** infinite ammo option changed handler */
	void InfiniteAmmoOptionChanged(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex);

	/** infinite clip option changed handler */
	void InfiniteClipOptionChanged(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex);

	/** freeze timer option changed handler */
	void FreezeTimerOptionChanged(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex);

	/** health regen option changed handler */
	void HealthRegenOptionChanged(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex);

	/** aim sensitivity option changed handler */
	void AimSensitivityOptionChanged(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex);

	/** controller vibration toggle handler */
	void ToggleVibration(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex);

	/** invert y axis option changed handler */
	void InvertYAxisOptionChanged(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex);

	/** full screen option changed handler */
	void FullScreenOptionChanged(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex);

	/** gamma correction option changed handler */
	void GammaOptionChanged(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex);

	/** try to match current resolution with selected index */
	int32 GetCurrentResolutionIndex(FIntPoint CurrentRes);

	/** Get current mouse y-axis inverted option index*/
	int32 GetCurrentMouseYAxisInvertedIndex();

	/** get current mouse sensitivity option index */
	int32 GetCurrentMouseSensitivityIndex();

	/** get current gamma index */
	int32 GetCurrentGammaIndex();

	/** get current user index out of PlayerOwner */
	int32 GetOwnerUserIndex() const;

	/** Get the persistence user associated with PlayerOwner*/
	USoldierPersistentUser* GetPersistentUser() const;

	/** Owning player controller */
	ULocalPlayer* PlayerOwner;

	/** holds vibration option menu item */
	TSharedPtr<FSoldierMenuItem> VibrationOption;

	/** holds invert y axis option menu item */
	TSharedPtr<FSoldierMenuItem> InvertYAxisOption;

	/** holds aim sensitivity option menu item */
	TSharedPtr<FSoldierMenuItem> AimSensitivityOption;

	/** holds mouse sensitivity option menu item */
	TSharedPtr<FSoldierMenuItem> VideoResolutionOption;

	/** holds graphics quality option menu item */
	TSharedPtr<FSoldierMenuItem> GraphicsQualityOption;

	/** holds gamma correction option menu item */
	TSharedPtr<FSoldierMenuItem> GammaOption;

	/** holds full screen option menu item */
	TSharedPtr<FSoldierMenuItem> FullScreenOption;

	/** graphics quality option */
	int32 GraphicsQualityOpt;

	/** minimum sensitivity index */
	int32 MinSensitivity;

	/** current sensitivity set in options */
	float SensitivityOpt;

	/** current gamma correction set in options */
	float GammaOpt;

	/** full screen setting set in options */
	EWindowMode::Type bFullScreenOpt;

	/** controller vibration setting set in options */
	uint8 bVibrationOpt : 1;

	/** invert mouse setting set in options */
	uint8 bInvertYAxisOpt : 1;

	/** resolution setting set in options */
	FIntPoint ResolutionOpt;

	/** available resolutions list */
	TArray<FIntPoint> Resolutions;

	/** style used for the shooter options */
	const struct FSoldierOptionsStyle *OptionsStyle;
};
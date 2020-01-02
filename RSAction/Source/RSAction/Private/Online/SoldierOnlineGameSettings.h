// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * General session settings for a Soldier game
 */
class FSoldierOnlineSessionSettings : public FOnlineSessionSettings
{
public:

	FSoldierOnlineSessionSettings(bool bIsLAN = false, bool bIsPresence = false, int32 MaxNumPlayers = 4);
	virtual ~FSoldierOnlineSessionSettings() {}
};

/**
 * General search setting for a Soldier game
 */
class FSoldierOnlineSearchSettings : public FOnlineSessionSearch
{
public:
	FSoldierOnlineSearchSettings(bool bSearchingLAN = false, bool bSearchingPresence = false);

	virtual ~FSoldierOnlineSearchSettings() {}
};

/**
 * Search settings for an empty dedicated server to host a match
 */
class FSoldierOnlineSearchSettingsEmptyDedicated : public FSoldierOnlineSearchSettings
{
public:
	FSoldierOnlineSearchSettingsEmptyDedicated(bool bSearchingLAN = false, bool bSearchingPresence = false);

	virtual ~FSoldierOnlineSearchSettingsEmptyDedicated() {}
};

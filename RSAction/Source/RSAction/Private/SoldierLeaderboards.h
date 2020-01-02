// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SoldierTypes.h"
#include "..\..\Engine\Plugins\Online\OnlineSubsystem\Source\Public\Interfaces\OnlineLeaderboardInterface.h"

// these are normally exported from platform-specific tools
#define LEADERBOARD_STAT_SCORE				"Score"
#define LEADERBOARD_STAT_KILLS				"Frags"
#define LEADERBOARD_STAT_DEATHS				"Deaths"
#define LEADERBOARD_STAT_MATCHESPLAYED		"MatchesPlayed"

/**
 *	'AllTime' leaderboard read object
 */
class FSoldierAllTimeMatchResultsRead : public FOnlineLeaderboardRead
{
public:

	FSoldierAllTimeMatchResultsRead()
	{
		// Default properties
		LeaderboardName = FName(TEXT("SoldierAllTimeMatchResults"));
		SortedColumn = LEADERBOARD_STAT_SCORE;

		// Define default columns
		new (ColumnMetadata) FColumnMetaData(LEADERBOARD_STAT_SCORE, EOnlineKeyValuePairDataType::Int32);
		new (ColumnMetadata) FColumnMetaData(LEADERBOARD_STAT_KILLS, EOnlineKeyValuePairDataType::Int32);
		new (ColumnMetadata) FColumnMetaData(LEADERBOARD_STAT_DEATHS, EOnlineKeyValuePairDataType::Int32);
		new (ColumnMetadata) FColumnMetaData(LEADERBOARD_STAT_MATCHESPLAYED, EOnlineKeyValuePairDataType::Int32);
	}
};

/**
 *	'AllTime' leaderboard write object
 */
class FSoldierAllTimeMatchResultsWrite : public FOnlineLeaderboardWrite
{
public:

	FSoldierAllTimeMatchResultsWrite()
	{
		// Default properties
		new (LeaderboardNames) FName(TEXT("SoldierAllTimeMatchResults"));
		RatedStat = LEADERBOARD_STAT_SCORE;
		DisplayFormat = ELeaderboardFormat::Number;
		SortMethod = ELeaderboardSort::Descending;
		UpdateMethod = ELeaderboardUpdateMethod::KeepBest;
	}
};


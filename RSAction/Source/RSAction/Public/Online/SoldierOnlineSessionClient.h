// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineSessionClient.h"
#include "SoldierOnlineSessionClient.generated.h"

UCLASS(Config = Game)
class USoldierOnlineSessionClient : public UOnlineSessionClient
{
	GENERATED_BODY()

public:
	/** Ctor */
	USoldierOnlineSessionClient();

	virtual void OnSessionUserInviteAccepted(
		const bool							bWasSuccess,
		const int32							ControllerId,
		TSharedPtr< const FUniqueNetId >	UserId,
		const FOnlineSessionSearchResult &	InviteResult
	) override;

	virtual void OnPlayTogetherEventReceived(int32 UserIndex, TArray<TSharedPtr<const FUniqueNetId>> UserIdList) override;

};

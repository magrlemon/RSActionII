// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "RSAction.h"
#include "SoldierOnlineSessionClient.h"
#include "SoldierGameInstance.h"

USoldierOnlineSessionClient::USoldierOnlineSessionClient()
{
}

void USoldierOnlineSessionClient::OnSessionUserInviteAccepted(
	const bool							bWasSuccess,
	const int32							ControllerId,
	TSharedPtr< const FUniqueNetId > 	UserId,
	const FOnlineSessionSearchResult &	InviteResult
)
{
	UE_LOG(LogOnline, Verbose, TEXT("HandleSessionUserInviteAccepted: bSuccess: %d, ControllerId: %d, User: %s"), bWasSuccess, ControllerId, UserId.IsValid() ? *UserId->ToString() : TEXT("NULL"));

	if (!bWasSuccess)
	{
		return;
	}

	if (!InviteResult.IsValid())
	{
		UE_LOG(LogOnline, Warning, TEXT("Invite accept returned no search result."));
		return;
	}

	if (!UserId.IsValid())
	{
		UE_LOG(LogOnline, Warning, TEXT("Invite accept returned no user."));
		return;
	}

	USoldierGameInstance* SoldierGameInstance = Cast<USoldierGameInstance>(GetGameInstance());

	if (SoldierGameInstance)
	{
		FSoldierPendingInvite PendingInvite;

		// Set the pending invite, and then go to the initial screen, which is where we will process it
		PendingInvite.ControllerId = ControllerId;
		PendingInvite.UserId = UserId;
		PendingInvite.InviteResult = InviteResult;
		PendingInvite.bPrivilegesCheckedAndAllowed = false;

		SoldierGameInstance->SetPendingInvite(PendingInvite);
		SoldierGameInstance->GotoState(SoldierGameInstanceState::PendingInvite);
	}
}

void USoldierOnlineSessionClient::OnPlayTogetherEventReceived(int32 UserIndex, TArray<TSharedPtr<const FUniqueNetId>> UserIdList)
{	
	if (USoldierGameInstance* const SoldierGameInstance = Cast<USoldierGameInstance>(GetGameInstance()))
	{
		SoldierGameInstance->OnPlayTogetherEventReceived(UserIndex, UserIdList);
	}
}
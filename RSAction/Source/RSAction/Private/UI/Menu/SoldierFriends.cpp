// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "RSAction.h"
#include "SoldierFriends.h"
#include "SoldierTypes.h"
#include "SoldierStyle.h"
#include "SoldierOptionsWidgetStyle.h"
#include "Player/SoldierPersistentUser.h"
#include "ShooterGameUserSettings.h"
#include "SoldierLocalPlayer.h"
#include "OnlineSubsystemUtils.h"

#define LOCTEXT_NAMESPACE "RSAction.HUD.Menu"

void FSoldierFriends::Construct(ULocalPlayer* _PlayerOwner, int32 LocalUserNum_)
{
	FriendsStyle = &FSoldierStyle::Get().GetWidgetStyle<FSoldierOptionsStyle>("DefaultSoldierOptionsStyle");

	PlayerOwner = _PlayerOwner;
	LocalUserNum = LocalUserNum_;
	CurrFriendIndex = 0;
	MinFriendIndex = 0;
	MaxFriendIndex = 0; //initialized after the friends list is read in

	/** Friends menu root item */
	TSharedPtr<FSoldierMenuItem> FriendsRoot = FSoldierMenuItem::CreateRoot();

	//Populate the friends list
	FriendsItem = MenuHelper::AddMenuItem(FriendsRoot, LOCTEXT("Friends", "FRIENDS"));

	if (PlayerOwner)
	{
		OnlineSub = Online::GetSubsystem(PlayerOwner->GetWorld());
		OnlineFriendsPtr = OnlineSub->GetFriendsInterface();
	}

	UpdateFriends(LocalUserNum);

	UserSettings = CastChecked<UShooterGameUserSettings>(GEngine->GetGameUserSettings());
}

void FSoldierFriends::OnApplySettings()
{
	ApplySettings();
}

void FSoldierFriends::ApplySettings()
{
	USoldierPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
		PersistentUser->TellInputAboutKeybindings();

		PersistentUser->SaveIfDirty();
	}

	UserSettings->ApplySettings(false);

	OnApplyChanges.ExecuteIfBound();
}

void FSoldierFriends::TellInputAboutKeybindings()
{
	USoldierPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
		PersistentUser->TellInputAboutKeybindings();
	}
}

USoldierPersistentUser* FSoldierFriends::GetPersistentUser() const
{
	USoldierLocalPlayer* const SoldierLocalPlayer = Cast<USoldierLocalPlayer>(PlayerOwner);
	return SoldierLocalPlayer ? SoldierLocalPlayer->GetPersistentUser() : nullptr;
	//// Main Menu
	//ASoldierPlayerController_Menu* SoldierPCM = Cast<ASoldierPlayerController_Menu>(PCOwner);
	//if(SoldierPCM)
	//{
	//	return SoldierPCM->GetPersistentUser();
	//}

	//// In-game Menu
	//ASoldierPlayerController* SoldierPC = Cast<ASoldierPlayerController>(PCOwner);
	//if(SoldierPC)
	//{
	//	return SoldierPC->GetPersistentUser();
	//}

	//return nullptr;
}

void FSoldierFriends::UpdateFriends(int32 NewOwnerIndex)
{
	if (!OnlineFriendsPtr.IsValid())
	{
		return;
	}

	LocalUserNum = NewOwnerIndex;
	OnlineFriendsPtr->ReadFriendsList(LocalUserNum, EFriendsLists::ToString(EFriendsLists::OnlinePlayers), FOnReadFriendsListComplete::CreateSP(this, &FSoldierFriends::OnFriendsUpdated));
}

void FSoldierFriends::OnFriendsUpdated(int32 /*unused*/, bool bWasSuccessful, const FString& FriendListName, const FString& ErrorString)
{
	if (!bWasSuccessful)
	{
		UE_LOG(LogOnline, Warning, TEXT("Unable to update friendslist %s due to error=[%s]"), *FriendListName, *ErrorString);
		return;
	}

	MenuHelper::ClearSubMenu(FriendsItem);

	Friends.Reset();
	if (OnlineFriendsPtr->GetFriendsList(LocalUserNum, EFriendsLists::ToString(EFriendsLists::OnlinePlayers), Friends))
	{
		for (const TSharedRef<FOnlineFriend> Friend : Friends)
		{
			TSharedRef<FSoldierMenuItem> FriendItem = MenuHelper::AddMenuItem(FriendsItem, FText::FromString(Friend->GetDisplayName()));
			FriendItem->OnControllerFacebuttonDownPressed.BindSP(this, &FSoldierFriends::ViewSelectedFriendProfile);
			FriendItem->OnControllerDownInputPressed.BindSP(this, &FSoldierFriends::IncrementFriendsCounter);
			FriendItem->OnControllerUpInputPressed.BindSP(this, &FSoldierFriends::DecrementFriendsCounter);
		}

		MaxFriendIndex = Friends.Num() - 1;
	}

	MenuHelper::AddMenuItemSP(FriendsItem, LOCTEXT("Close", "CLOSE"), this, &FSoldierFriends::OnApplySettings);
}

void FSoldierFriends::IncrementFriendsCounter()
{
	if (CurrFriendIndex + 1 <= MaxFriendIndex)
	{
		++CurrFriendIndex;
	}
}
void FSoldierFriends::DecrementFriendsCounter()
{
	if (CurrFriendIndex - 1 >= MinFriendIndex)
	{
		--CurrFriendIndex;
	}
}
void FSoldierFriends::ViewSelectedFriendProfile()
{
	if (OnlineSub)
	{
		IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
		if (Identity.IsValid() && Friends.IsValidIndex(CurrFriendIndex))
		{
			TSharedPtr<const FUniqueNetId> Requestor = Identity->GetUniquePlayerId(LocalUserNum);
			TSharedPtr<const FUniqueNetId> Requestee = Friends[CurrFriendIndex]->GetUserId();
			
			IOnlineExternalUIPtr ExternalUI = OnlineSub->GetExternalUIInterface();
			if (ExternalUI.IsValid() && Requestor.IsValid() && Requestee.IsValid())
			{
				ExternalUI->ShowProfileUI(*Requestor, *Requestee, FOnProfileUIClosedDelegate());
			}
		}
}
}
void FSoldierFriends::InviteSelectedFriendToGame()
{
	// invite the user to the current gamesession
	if (OnlineSub)
	{
		IOnlineSessionPtr OnlineSessionInterface = OnlineSub->GetSessionInterface();
		if (OnlineSessionInterface.IsValid())
		{
			OnlineSessionInterface->SendSessionInviteToFriend(LocalUserNum, NAME_GameSession, *Friends[CurrFriendIndex]->GetUserId());
		}
	}
}


#undef LOCTEXT_NAMESPACE


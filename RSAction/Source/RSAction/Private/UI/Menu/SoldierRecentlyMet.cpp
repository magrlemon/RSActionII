// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "RSAction.h"
#include "SoldierRecentlyMet.h"
#include "SoldierTypes.h"
#include "SoldierStyle.h"
#include "SoldierOptionsWidgetStyle.h"
#include "SoldierGameUserSettings.h"
#include "SoldierPersistentUser.h"
#include "Player/SoldierLocalPlayer.h"
#include "OnlineSubsystemUtils.h"

#define LOCTEXT_NAMESPACE "RSAction.HUD.Menu"

void FSoldierRecentlyMet::Construct(ULocalPlayer* _PlayerOwner, int32 LocalUserNum_)
{
	RecentlyMetStyle = &FSoldierStyle::Get().GetWidgetStyle<FSoldierOptionsStyle>("DefaultSoldierOptionsStyle");

	PlayerOwner = _PlayerOwner;
	LocalUserNum = LocalUserNum_;
	CurrRecentlyMetIndex = 0;
	MinRecentlyMetIndex = 0;
	MaxRecentlyMetIndex = 0; //initialized after the recently met list is (read in/set)

	/** Recently Met menu items */
	RecentlyMetRoot = FSoldierMenuItem::CreateRoot();
	RecentlyMetItem = MenuHelper::AddMenuItem(RecentlyMetRoot, LOCTEXT("Recently Met", "RECENTLY MET"));

	/** Init online items */
	if (PlayerOwner)
	{
		OnlineSub = Online::GetSubsystem(PlayerOwner->GetWorld());
	}

	UserSettings = CastChecked<USoldierGameUserSettings>(GEngine->GetGameUserSettings());	
}

void FSoldierRecentlyMet::OnApplySettings()
{
	ApplySettings();
}

void FSoldierRecentlyMet::ApplySettings()
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

void FSoldierRecentlyMet::TellInputAboutKeybindings()
{
	USoldierPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
		PersistentUser->TellInputAboutKeybindings();
	}
}

USoldierPersistentUser* FSoldierRecentlyMet::GetPersistentUser() const
{
	USoldierLocalPlayer* const SLP = Cast<USoldierLocalPlayer>(PlayerOwner);
	return SLP ? SLP->GetPersistentUser() : nullptr;
}

void FSoldierRecentlyMet::UpdateRecentlyMet(int32 NewOwnerIndex)
{
	LocalUserNum = NewOwnerIndex;
	
	if (OnlineSub)
	{
		IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
		if (Identity.IsValid())
		{
			LocalUsername = Identity->GetPlayerNickname(LocalUserNum);
		}
	}
	
	MenuHelper::ClearSubMenu(RecentlyMetItem);
	MaxRecentlyMetIndex = 0;

	ASoldierGameState* const MyGameState = PlayerOwner->GetWorld()->GetGameState<ASoldierGameState>();
	if (MyGameState != nullptr)
	{
		MetPlayerArray = MyGameState->PlayerArray;

		for (int32 i = 0; i < MetPlayerArray.Num(); ++i)
		{
			const APlayerState* PlayerState = MetPlayerArray[i];
			FString Username = PlayerState->GetHumanReadableName();
			if (Username != LocalUsername && PlayerState->bIsABot == false)
			{
				TSharedPtr<FSoldierMenuItem> UserItem = MenuHelper::AddMenuItem(RecentlyMetItem, FText::FromString(Username));
				UserItem->OnControllerDownInputPressed.BindRaw(this, &FSoldierRecentlyMet::IncrementRecentlyMetCounter);
				UserItem->OnControllerUpInputPressed.BindRaw(this, &FSoldierRecentlyMet::DecrementRecentlyMetCounter);
				UserItem->OnControllerFacebuttonDownPressed.BindRaw(this, &FSoldierRecentlyMet::ViewSelectedUsersProfile);
			}
			else
			{
				MetPlayerArray.RemoveAt(i);
				--i; //we just deleted an item, so we need to go make sure i doesn't increment again, otherwise it would skip the player that was supposed to be looked at next
			}
		}

		MaxRecentlyMetIndex = MetPlayerArray.Num() - 1;
	}

	MenuHelper::AddMenuItemSP(RecentlyMetItem, LOCTEXT("Close", "CLOSE"), this, &FSoldierRecentlyMet::OnApplySettings);
}

void FSoldierRecentlyMet::IncrementRecentlyMetCounter()
{
	if (CurrRecentlyMetIndex + 1 <= MaxRecentlyMetIndex)
	{
		++CurrRecentlyMetIndex;
	}
}
void FSoldierRecentlyMet::DecrementRecentlyMetCounter()
{
	if (CurrRecentlyMetIndex - 1 >= MinRecentlyMetIndex)
	{
		--CurrRecentlyMetIndex;
	}
}
void FSoldierRecentlyMet::ViewSelectedUsersProfile()
{
	if (OnlineSub)
	{
		IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
		if (Identity.IsValid() && MetPlayerArray.IsValidIndex(CurrRecentlyMetIndex))
		{
			const APlayerState* PlayerState = MetPlayerArray[CurrRecentlyMetIndex];
		
			TSharedPtr<const FUniqueNetId> Requestor = Identity->GetUniquePlayerId(LocalUserNum);
			TSharedPtr<const FUniqueNetId> Requestee = PlayerState->UniqueId.GetUniqueNetId();
			
			IOnlineExternalUIPtr ExternalUI = OnlineSub->GetExternalUIInterface();
			if (ExternalUI.IsValid() && Requestor.IsValid() && Requestee.IsValid())
			{
				ExternalUI->ShowProfileUI(*Requestor, *Requestee, FOnProfileUIClosedDelegate());
			}
		}
	}
}


#undef LOCTEXT_NAMESPACE

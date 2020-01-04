// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "RSAction.h"
#include "SoldierIngameMenu.h"
#include "SoldierStyle.h"
#include "SoldierMenuSoundsWidgetStyle.h"
#include "Online.h"
#include "OnlineExternalUIInterface.h"
#include "ShooterGameInstance.h"
#include "UI/SoldierHUD.h"
#include "OnlineSubsystemUtils.h"

#define LOCTEXT_NAMESPACE "RSAction.HUD.Menu"

#if PLATFORM_SWITCH
#	define FRIENDS_SUPPORTED 0
#else
#	define FRIENDS_SUPPORTED 1
#endif


void FSoldierIngameMenu::Construct(ULocalPlayer* _PlayerOwner)
{
	PlayerOwner = _PlayerOwner;
	bIsGameMenuUp = false;

	if (!GEngine || !GEngine->GameViewport)
	{
		return;
	}
	
	//todo:  don't create ingame menus for remote players.
	const UShooterGameInstance* GameInstance = nullptr;
	if (PlayerOwner)
	{
		GameInstance = Cast<UShooterGameInstance>(PlayerOwner->GetGameInstance());
	}

	if (!GameMenuWidget.IsValid())
	{
		SAssignNew(GameMenuWidget, SSoldierMenuWidget)
			.PlayerOwner(MakeWeakObjectPtr(PlayerOwner))
			.Cursor(EMouseCursor::Default)
			.IsGameMenu(true);			


		int32 const OwnerUserIndex = GetOwnerUserIndex();

		// setup the exit to main menu submenu.  We wanted a confirmation to avoid a potential TRC violation.
		// fixes TTP: 322267
		TSharedPtr<FSoldierMenuItem> MainMenuRoot = FSoldierMenuItem::CreateRoot();
		MainMenuItem = MenuHelper::AddMenuItem(MainMenuRoot,LOCTEXT("Main Menu", "MAIN MENU"));
		MenuHelper::AddMenuItemSP(MainMenuItem,LOCTEXT("No", "NO"), this, &FSoldierIngameMenu::OnCancelExitToMain);
		MenuHelper::AddMenuItemSP(MainMenuItem,LOCTEXT("Yes", "YES"), this, &FSoldierIngameMenu::OnConfirmExitToMain);		

		SoldierOptions = MakeShareable(new FSoldierOptions());
		SoldierOptions->Construct(PlayerOwner);
		SoldierOptions->TellInputAboutKeybindings();
		SoldierOptions->OnApplyChanges.BindSP(this, &FSoldierIngameMenu::CloseSubMenu);

		MenuHelper::AddExistingMenuItem(RootMenuItem, SoldierOptions->CheatsItem.ToSharedRef());
		MenuHelper::AddExistingMenuItem(RootMenuItem, SoldierOptions->OptionsItem.ToSharedRef());

#if FRIENDS_SUPPORTED
		if (GameInstance && GameInstance->GetOnlineMode() == EOnlineMode::Online)
		{
#if !PLATFORM_XBOXONE
			SoldierFriends = MakeShareable(new FSoldierFriends());
			SoldierFriends->Construct(PlayerOwner, OwnerUserIndex);
			SoldierFriends->TellInputAboutKeybindings();
			SoldierFriends->OnApplyChanges.BindSP(this, &FSoldierIngameMenu::CloseSubMenu);

			MenuHelper::AddExistingMenuItem(RootMenuItem, SoldierFriends->FriendsItem.ToSharedRef());

			SoldierRecentlyMet = MakeShareable(new FSoldierRecentlyMet());
			SoldierRecentlyMet->Construct(PlayerOwner, OwnerUserIndex);
			SoldierRecentlyMet->TellInputAboutKeybindings();
			SoldierRecentlyMet->OnApplyChanges.BindSP(this, &FSoldierIngameMenu::CloseSubMenu);

			MenuHelper::AddExistingMenuItem(RootMenuItem, SoldierRecentlyMet->RecentlyMetItem.ToSharedRef());
#endif		

#if SHOOTER_CONSOLE_UI			
			TSharedPtr<FSoldierMenuItem> ShowInvitesItem = MenuHelper::AddMenuItem(RootMenuItem, LOCTEXT("Invite Players", "INVITE PLAYERS (via System UI)"));
			ShowInvitesItem->OnConfirmMenuItem.BindRaw(this, &FSoldierIngameMenu::OnShowInviteUI);		
#endif
		}
#endif

		if (FSlateApplication::Get().SupportsSystemHelp())
		{
			TSharedPtr<FSoldierMenuItem> HelpSubMenu = MenuHelper::AddMenuItem(RootMenuItem, LOCTEXT("Help", "HELP"));
			HelpSubMenu->OnConfirmMenuItem.BindStatic([](){ FSlateApplication::Get().ShowSystemHelp(); });
		}

		MenuHelper::AddExistingMenuItem(RootMenuItem, MainMenuItem.ToSharedRef());
				
#if !SHOOTER_CONSOLE_UI
		MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("Quit", "QUIT"), this, &FSoldierIngameMenu::OnUIQuit);
#endif

		GameMenuWidget->MainMenu = GameMenuWidget->CurrentMenu = RootMenuItem->SubMenu;
		GameMenuWidget->OnMenuHidden.BindSP(this,&FSoldierIngameMenu::DetachGameMenu);
		GameMenuWidget->OnToggleMenu.BindSP(this,&FSoldierIngameMenu::ToggleGameMenu);
		GameMenuWidget->OnGoBack.BindSP(this, &FSoldierIngameMenu::OnMenuGoBack);
	}
}

void FSoldierIngameMenu::CloseSubMenu()
{
	GameMenuWidget->MenuGoBack();
}

void FSoldierIngameMenu::OnMenuGoBack(MenuPtr Menu)
{
	// if we are going back from options menu
	if (SoldierOptions.IsValid() && SoldierOptions->OptionsItem->SubMenu == Menu)
	{
		SoldierOptions->RevertChanges();
	}
}

bool FSoldierIngameMenu::GetIsGameMenuUp() const
{
	return bIsGameMenuUp;
}

void FSoldierIngameMenu::UpdateFriendsList()
{
	if (PlayerOwner)
	{
		IOnlineSubsystem* OnlineSub = Online::GetSubsystem(PlayerOwner->GetWorld());
		if (OnlineSub)
		{
			IOnlineFriendsPtr OnlineFriendsPtr = OnlineSub->GetFriendsInterface();
			if (OnlineFriendsPtr.IsValid())
			{
				OnlineFriendsPtr->ReadFriendsList(GetOwnerUserIndex(), EFriendsLists::ToString(EFriendsLists::OnlinePlayers));
			}
		}
	}
}

void FSoldierIngameMenu::DetachGameMenu()
{
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(GameMenuContainer.ToSharedRef());
	}
	bIsGameMenuUp = false;

	ASoldierPlayerController* const PCOwner = PlayerOwner ? Cast<ASoldierPlayerController>(PlayerOwner->PlayerController) : nullptr;
	if (PCOwner)
	{
		PCOwner->SetPause(false);

		// If the game is over enable the scoreboard
		ASoldierHUD* const SoldierHUD = PCOwner->GetSoldierHUD();
		if( ( SoldierHUD != NULL ) && ( SoldierHUD->IsMatchOver() == true ) && ( PCOwner->IsPrimaryPlayer() == true ) )
		{
			SoldierHUD->ShowScoreboard( true, true );
		}
	}
}

void FSoldierIngameMenu::ToggleGameMenu()
{
	//Update the owner in case the menu was opened by another controller
	//UpdateMenuOwner();

	if (!GameMenuWidget.IsValid())
	{
		return;
	}

	// check for a valid user index.  could be invalid if the user signed out, in which case the 'please connect your control' ui should be up anyway.
	// in-game menu needs a valid userindex for many OSS calls.
	if (GetOwnerUserIndex() == -1)
	{
		UE_LOG(LogShooter, Log, TEXT("Trying to toggle in-game menu for invalid userid"));
		return;
	}

	if (bIsGameMenuUp && GameMenuWidget->CurrentMenu != RootMenuItem->SubMenu)
	{
		GameMenuWidget->MenuGoBack();
		return;
	}
	
	ASoldierPlayerController* const PCOwner = PlayerOwner ? Cast<ASoldierPlayerController>(PlayerOwner->PlayerController) : nullptr;
	if (!bIsGameMenuUp)
	{
		// Hide the scoreboard
		if (PCOwner)
		{
			ASoldierHUD* const SoldierHUD = PCOwner->GetSoldierHUD();
			if( SoldierHUD != NULL )
			{
				SoldierHUD->ShowScoreboard( false );
			}
		}

		GEngine->GameViewport->AddViewportWidgetContent(
			SAssignNew(GameMenuContainer,SWeakWidget)
			.PossiblyNullContent(GameMenuWidget.ToSharedRef())
			);

		int32 const OwnerUserIndex = GetOwnerUserIndex();
		if(SoldierOptions.IsValid())
		{
			SoldierOptions->UpdateOptions();
		}
		if(SoldierRecentlyMet.IsValid())
		{
			SoldierRecentlyMet->UpdateRecentlyMet(OwnerUserIndex);
		}
		GameMenuWidget->BuildAndShowMenu();
		bIsGameMenuUp = true;

		if (PCOwner)
		{
			// Disable controls while paused
			PCOwner->SetCinematicMode(true, false, false, true, true);

			PCOwner->SetPause(true);

			FInputModeGameAndUI InputMode;
			PCOwner->SetInputMode(InputMode);
		}
	} 
	else
	{
		//Start hiding animation
		GameMenuWidget->HideMenu();
		if (PCOwner)
		{
			// Make sure viewport has focus
			FSlateApplication::Get().SetAllUserFocusToGameViewport();

			// Don't renable controls if the match is over
			ASoldierHUD* const SoldierHUD = PCOwner->GetSoldierHUD();
			if( ( SoldierHUD != NULL ) && ( SoldierHUD->IsMatchOver() == false ) )
			{
				PCOwner->SetCinematicMode(false,false,false,true,true);

				FInputModeGameOnly InputMode;
				PCOwner->SetInputMode(InputMode);
			}
		}
	}
}

void FSoldierIngameMenu::OnCancelExitToMain()
{
	CloseSubMenu();
}

void FSoldierIngameMenu::OnConfirmExitToMain()
{
	UShooterGameInstance* const GameInstance = Cast<UShooterGameInstance>(PlayerOwner->GetGameInstance());
	if (GameInstance)
	{
		GameInstance->LabelPlayerAsQuitter(PlayerOwner);

		// tell game instance to go back to main menu state
		GameInstance->GotoState(ShooterGameInstanceState::MainMenu);
	}
}

void FSoldierIngameMenu::OnUIQuit()
{
	UShooterGameInstance* const GI = Cast<UShooterGameInstance>(PlayerOwner->GetGameInstance());
	if (GI)
	{
		GI->LabelPlayerAsQuitter(PlayerOwner);
	}

	GameMenuWidget->LockControls(true);
	GameMenuWidget->HideMenu();

	UWorld* const World = PlayerOwner ? PlayerOwner->GetWorld() : nullptr;
	if (World)
	{
		const FSoldierMenuSoundsStyle& MenuSounds = FSoldierStyle::Get().GetWidgetStyle<FSoldierMenuSoundsStyle>("DefaultSoldierMenuSoundsStyle");
		MenuHelper::PlaySoundAndCall(World, MenuSounds.ExitGameSound, GetOwnerUserIndex(), this, &FSoldierIngameMenu::Quit);
	}
}

void FSoldierIngameMenu::Quit()
{
	APlayerController* const PCOwner = PlayerOwner ? PlayerOwner->PlayerController : nullptr;
	if (PCOwner)
	{
		PCOwner->ConsoleCommand("quit");
	}
}

void FSoldierIngameMenu::OnShowInviteUI()
{
	if (PlayerOwner)
	{
		const IOnlineExternalUIPtr ExternalUI = Online::GetExternalUIInterface(PlayerOwner->GetWorld());

		if (!ExternalUI.IsValid())
		{
			UE_LOG(LogShooter, Warning, TEXT("OnShowInviteUI: External UI interface is not supported on this platform."));
			return;
		}

		ExternalUI->ShowInviteUI(GetOwnerUserIndex());
	}
}

int32 FSoldierIngameMenu::GetOwnerUserIndex() const
{
	return PlayerOwner ? PlayerOwner->GetControllerId() : 0;
}


#undef LOCTEXT_NAMESPACE

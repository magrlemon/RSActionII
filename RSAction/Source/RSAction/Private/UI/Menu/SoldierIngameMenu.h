// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "SlateBasics.h"
#include "SlateExtras.h"
#include "Widgets/SoldierMenuItem.h"
#include "Widgets/SSoldierMenuWidget.h"
#include "SoldierOptions.h"
#include "SoldierFriends.h"
#include "SoldierRecentlyMet.h"

class FSoldierIngameMenu : public TSharedFromThis<FSoldierIngameMenu>
{
public:
	/** sets owning player controller */
	void Construct(ULocalPlayer* PlayerOwner);

	/** toggles in game menu */
	void ToggleGameMenu();

	/** is game menu currently active? */
	bool GetIsGameMenuUp() const;

	/* updates the friends list of the current owner*/
	void UpdateFriendsList();

	/* Getter for the SoldierFriends interface/pointer*/
	TSharedPtr<class FSoldierFriends> GetSoldierFriends(){ return SoldierFriends; }

protected:

	/** Owning player controller */
	ULocalPlayer* PlayerOwner;

	/** game menu container widget - used for removing */
	TSharedPtr<class SWeakWidget> GameMenuContainer;

	/** root menu item pointer */
	TSharedPtr<FSoldierMenuItem> RootMenuItem;

	/** main menu item pointer */
	TSharedPtr<FSoldierMenuItem> MainMenuItem;

	/** HUD menu widget */
	TSharedPtr<class SSoldierMenuWidget> GameMenuWidget;	

	/** if game menu is currently opened*/
	bool bIsGameMenuUp;

	/** holds cheats menu item to toggle it's visibility */
	TSharedPtr<class FSoldierMenuItem> CheatsMenu;

	/** Soldier options */
	TSharedPtr<class FSoldierOptions> SoldierOptions;

	/** get current user index out of PlayerOwner */
	int32 GetOwnerUserIndex() const;
	/** Soldier friends */
	TSharedPtr<class FSoldierFriends> SoldierFriends;

	/** Soldier recently met users*/
	TSharedPtr<class FSoldierRecentlyMet> SoldierRecentlyMet;

	/** called when going back to previous menu */
	void OnMenuGoBack(MenuPtr Menu);
	
	/** goes back in menu structure */
	void CloseSubMenu();

	/** removes widget from viewport */
	void DetachGameMenu();
	
	/** Delegate called when user cancels confirmation dialog to exit to main menu */
	void OnCancelExitToMain();

	/** Delegate called when user confirms confirmation dialog to exit to main menu */
	void OnConfirmExitToMain();		

	/** Plays sound and calls Quit */
	void OnUIQuit();

	/** Quits the game */
	void Quit();

	/** Shows the system UI to invite friends to the game */
	void OnShowInviteUI();
};

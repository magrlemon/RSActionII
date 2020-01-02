// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "SlateBasics.h"
#include "SlateExtras.h"
#include "SSoldierMenuItem.h"

namespace ESoldierMenuItemType
{
	enum Type
	{
		Root,
		Standard,
		MultiChoice,
		CustomWidget,
	};
};

/** TArray< TSharedPtr<class FSoldierMenuItem> > */
typedef TArray< TSharedPtr<class FSoldierMenuItem> > MenuPtr;

class FSoldierMenuInfo 
{
public:
	/** menu items array */
	MenuPtr Menu;

	/** last selection in this menu */
	int32 SelectedIndex;

	/** menu title */
	FText MenuTitle;

	/** constructor making filling required information easy */
	FSoldierMenuInfo(MenuPtr _Menu, int32 _SelectedIndex, FText _MenuTitle)
	{
		Menu = _Menu;
		SelectedIndex = _SelectedIndex;
		MenuTitle = MoveTemp(_MenuTitle);
	}
};

class FSoldierMenuItem : public TSharedFromThis<FSoldierMenuItem>
{
public:
	/** confirm menu item delegate */
	DECLARE_DELEGATE(FOnConfirmMenuItem);

	/** view profile delegate */
	DECLARE_DELEGATE(FOnControllerFacebuttonLeftPressed);

	/** Increment friend index counter delegate */
	DECLARE_DELEGATE(FOnControllerDownInputPressed);

	/** Decrement friend index counter delegate */
	DECLARE_DELEGATE(FOnControllerUpInputPressed);

	/** Send friend invite delegate */
	DECLARE_DELEGATE(FOnOnControllerFacebuttonDownPressed);
	
	/** multi-choice option changed, parameters are menu item itself and new multi-choice index  */
	DECLARE_DELEGATE_TwoParams(FOnOptionChanged, TSharedPtr<FSoldierMenuItem>, int32);

	/** delegate, which is executed by SSoldierMenuWidget if user confirms this menu item */
	FOnConfirmMenuItem OnConfirmMenuItem;

	/** multi-choice option changed, parameters are menu item itself and new multi-choice index */
	FOnOptionChanged OnOptionChanged;

	/** delegate, which is executed by SSoldierMenuWidget if user presses FacebuttonLeft */
	FOnControllerFacebuttonLeftPressed OnControllerFacebuttonLeftPressed;

	/** delegate, which is executed by SSoldierMenuWidget if user presses ControllerDownInput */
	FOnControllerDownInputPressed OnControllerDownInputPressed;

	/** delegate, which is executed by SSoldierMenuWidget if user presses ControllerUpInput */
	FOnControllerUpInputPressed OnControllerUpInputPressed;

	/** delegate, which is executed by SSoldierMenuWidget if user presses FacebuttonDown */
	FOnOnControllerFacebuttonDownPressed OnControllerFacebuttonDownPressed;

	/** menu item type */
	ESoldierMenuItemType::Type MenuItemType;	

	/** if this menu item will be created when menu is opened */
	bool bVisible;

	/** sub menu if present */
	TArray< TSharedPtr<FSoldierMenuItem> > SubMenu;

	/** shared pointer to actual slate widget representing the menu item */
	TSharedPtr<SSoldierMenuItem> Widget;

	/** shared pointer to actual slate widget representing the custom menu item, ie whole options screen */
	TSharedPtr<SWidget> CustomWidget;

	/** texts for multiple choice menu item (like INF AMMO ON/OFF or difficulty/resolution etc) */
	TArray<FText> MultiChoice;

	/** set to other value than -1 to limit the options range */
	int32 MinMultiChoiceIndex;

	/** set to other value than -1 to limit the options range */
	int32 MaxMultiChoiceIndex;

	/** selected multi-choice index for this menu item */
	int32 SelectedMultiChoice;

	/** constructor accepting menu item text */
	FSoldierMenuItem(FText _text)
	{
		bVisible = true;
		Text = MoveTemp(_text);
		MenuItemType = ESoldierMenuItemType::Standard;
	}

	/** custom widgets cannot contain sub menus, all functionality must be handled by custom widget itself */
	FSoldierMenuItem(TSharedPtr<SWidget> _Widget)
	{
		bVisible = true;
		MenuItemType = ESoldierMenuItemType::CustomWidget;
		CustomWidget = _Widget;
	}

	/** constructor for multi-choice item */
	FSoldierMenuItem(FText _text, TArray<FText> _choices, int32 DefaultIndex=0)
	{
		bVisible = true;
		Text = MoveTemp(_text);
		MenuItemType = ESoldierMenuItemType::MultiChoice;
		MultiChoice = MoveTemp(_choices);
		MinMultiChoiceIndex = MaxMultiChoiceIndex = -1;
		SelectedMultiChoice = DefaultIndex;
	}

	const FText& GetText() const
	{
		return Text;
	}

	void SetText(FText UpdatedText)
	{
		Text = MoveTemp(UpdatedText);
		if (Widget.IsValid())
		{
			Widget->UpdateItemText(Text);
		}
	}

	/** create special root item */
	static TSharedRef<FSoldierMenuItem> CreateRoot()
	{
		return MakeShareable(new FSoldierMenuItem());
	}

private:

	/** menu item text */
	FText Text;

	FSoldierMenuItem()
	{
		bVisible = false;
		MenuItemType = ESoldierMenuItemType::Root;
	}
};
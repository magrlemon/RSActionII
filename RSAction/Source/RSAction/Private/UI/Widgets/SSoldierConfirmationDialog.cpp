// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "RSAction.h"
#include "SoldierStyle.h"
#include "SSoldierConfirmationDialog.h"
#include "SoldierMenuItemWidgetStyle.h"
#include "SoldierGameInstance.h"
#include "OnlineSubsystemUtils.h"

void SSoldierConfirmationDialog::Construct( const FArguments& InArgs )
{	
	PlayerOwner = InArgs._PlayerOwner;
	DialogType = InArgs._DialogType;

	OnConfirm = InArgs._OnConfirmClicked;
	OnCancel = InArgs._OnCancelClicked;

	const FSoldierMenuItemStyle* ItemStyle = &FSoldierStyle::Get().GetWidgetStyle<FSoldierMenuItemStyle>("DefaultSoldierMenuItemStyle");
	const FButtonStyle* ButtonStyle = &FSoldierStyle::Get().GetWidgetStyle<FButtonStyle>("DefaultSoldierButtonStyle");
	FLinearColor MenuTitleTextColor =  FLinearColor(FColor(155,164,182));
	ChildSlot
	.VAlign(VAlign_Center)
	.HAlign(HAlign_Center)
	[					
		SNew( SVerticalBox )
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(20.0f)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		[
			SNew(SBorder)
			.Padding(50.0f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.BorderImage(&ItemStyle->BackgroundBrush)
			.BorderBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
			[
				SNew( STextBlock )
				.TextStyle(FSoldierStyle::Get(), "SoldierGame.MenuHeaderTextStyle")
				.ColorAndOpacity(MenuTitleTextColor)
				.Text(InArgs._MessageText)
				.WrapTextAt(500.0f)
			]
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.Padding(20.0f)
		[
			SNew( SHorizontalBox)
			+SHorizontalBox::Slot()			
			.AutoWidth()
			.Padding(20.0f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew( SButton )
				.ContentPadding(100)
				.OnClicked(this, &SSoldierConfirmationDialog::OnConfirmHandler)
				.Text(InArgs._ConfirmText)			
				.TextStyle(FSoldierStyle::Get(), "SoldierGame.MenuHeaderTextStyle")
				.ButtonStyle(ButtonStyle)
				.IsFocusable(false)
			]

			+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(20.0f)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew( SButton )
					.ContentPadding(100)
					.OnClicked(InArgs._OnCancelClicked)
					.Text(InArgs._CancelText)
					.TextStyle(FSoldierStyle::Get(), "SoldierGame.MenuHeaderTextStyle")
					.ButtonStyle(ButtonStyle)
					.Visibility(InArgs._CancelText.IsEmpty() == false ? EVisibility::Visible : EVisibility::Collapsed)
					.IsFocusable(false)
				]	
		]			
	];
}

bool SSoldierConfirmationDialog::SupportsKeyboardFocus() const
{
	return true;
}

FReply SSoldierConfirmationDialog::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
{
	return FReply::Handled().ReleaseMouseCapture().SetUserFocus(SharedThis(this), EFocusCause::SetDirectly, true);
}

FReply SSoldierConfirmationDialog::OnConfirmHandler()
{
	return ExecuteConfirm(FSlateApplication::Get().GetUserIndexForKeyboard());
}

FReply SSoldierConfirmationDialog::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	const FKey Key = KeyEvent.GetKey();
	const int32 UserIndex = KeyEvent.GetUserIndex();

	// Filter input based on the type of this dialog
	switch (DialogType)
	{
		case ESoldierDialogType::Generic:
		{
			// Ignore input from users that don't own this dialog
			if (PlayerOwner != nullptr && PlayerOwner->GetControllerId() != UserIndex)
			{
				return FReply::Unhandled();
			}
			break;
		}

		case ESoldierDialogType::ControllerDisconnected:
		{
			// Only ignore input from controllers that are bound to local users
			if(PlayerOwner != nullptr && PlayerOwner->GetGameInstance() != nullptr)
			{
				if (PlayerOwner->GetGameInstance()->FindLocalPlayerFromControllerId(UserIndex))
				{
					return FReply::Unhandled();
				}
			}
			break;
		}
	}

	// For testing on PC
	if ((Key == EKeys::Enter || Key == EKeys::Virtual_Accept) && !KeyEvent.IsRepeat())
	{
		return ExecuteConfirm(UserIndex);
	}
	else if (Key == EKeys::Escape || Key == EKeys::Virtual_Back)
	{
		if(OnCancel.IsBound())
		{
			return OnCancel.Execute();
		}
	}

	return FReply::Unhandled();
}

FReply SSoldierConfirmationDialog::ExecuteConfirm(const int32 UserIndex)
{
	if (OnConfirm.IsBound())
	{
		//these two cases should be combined when we move to using PlatformUserIDs rather than ControllerIDs.
#if PLATFORM_PS4
		bool bExecute = false;
		// For controller reconnection, bind the confirming controller to the owner of this dialog
		if (DialogType == ESoldierDialogType::ControllerDisconnected && PlayerOwner != nullptr)
		{
			const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(PlayerOwner->GetWorld());
			if (OnlineSub)
			{
				const IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
				if (IdentityInterface.IsValid())
				{
					TSharedPtr<const FUniqueNetId> IncomingUserId = IdentityInterface->GetUniquePlayerId(UserIndex);
					FUniqueNetIdRepl DisconnectedId = PlayerOwner->GetCachedUniqueNetId();

					if (*IncomingUserId == *DisconnectedId)
					{
						PlayerOwner->SetControllerId(UserIndex);
						bExecute = true;
					}
				}
			}
		}
		else
		{
			bExecute = true;
		}

		if (bExecute)
		{
			return OnConfirm.Execute();
		}
#else
		// For controller reconnection, bind the confirming controller to the owner of this dialog
		if (DialogType == ESoldierDialogType::ControllerDisconnected && PlayerOwner != nullptr)
		{
			PlayerOwner->SetControllerId(UserIndex);
		}

		return OnConfirm.Execute();
#endif
	}

	return FReply::Unhandled();
}
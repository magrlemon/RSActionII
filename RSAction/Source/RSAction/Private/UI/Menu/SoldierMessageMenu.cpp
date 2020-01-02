// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "RSAction.h"
#include "SoldierStyle.h"
#include "SSoldierConfirmationDialog.h"
#include "SoldierMessageMenu.h"
#include "RSActionGameViewportClient.h"
#include "RSActionGameInstance.h"

#define LOCTEXT_NAMESPACE "SoldierGame.HUD.Menu"

void FSoldierMessageMenu::Construct(TWeakObjectPtr<URSActionGameInstance> InGameInstance, TWeakObjectPtr<ULocalPlayer> InPlayerOwner, const FText& Message, const FText& OKButtonText, const FText& CancelButtonText, const FName& InPendingNextState)
{
	GameInstance			= InGameInstance;
	PlayerOwner				= InPlayerOwner;
	PendingNextState		= InPendingNextState;

	if ( ensure( GameInstance.IsValid() ) )
	{
		USoldierGameViewportClient* SoldierViewport = Cast<USoldierGameViewportClient>( GameInstance->GetGameViewportClient() );

		if ( SoldierViewport )
		{
			// Hide the previous dialog
			SoldierViewport->HideDialog();

			// Show the new one
			SoldierViewport->ShowDialog( 
				PlayerOwner,
				ESoldierDialogType::Generic,
				Message, 
				OKButtonText, 
				CancelButtonText, 
				FOnClicked::CreateRaw(this, &FSoldierMessageMenu::OnClickedOK),
				FOnClicked::CreateRaw(this, &FSoldierMessageMenu::OnClickedCancel)
			);
		}
	}
}

void FSoldierMessageMenu::RemoveFromGameViewport()
{
	if ( ensure( GameInstance.IsValid() ) )
	{
		USoldierGameViewportClient * SoldierViewport = Cast<USoldierGameViewportClient>( GameInstance->GetGameViewportClient() );

		if ( SoldierViewport )
		{
			// Hide the previous dialog
			SoldierViewport->HideDialog();
		}
	}
}

void FSoldierMessageMenu::HideDialogAndGotoNextState()
{
	RemoveFromGameViewport();

	if ( ensure( GameInstance.IsValid() ) )
	{
		GameInstance->GotoState( PendingNextState );
	}
};

FReply FSoldierMessageMenu::OnClickedOK()
{
	OKButtonDelegate.ExecuteIfBound();
	HideDialogAndGotoNextState();
	return FReply::Handled();
}

FReply FSoldierMessageMenu::OnClickedCancel()
{
	CancelButtonDelegate.ExecuteIfBound();
	HideDialogAndGotoNextState();
	return FReply::Handled();
}

void FSoldierMessageMenu::SetOKClickedDelegate(FMessageMenuButtonClicked InButtonDelegate)
{
	OKButtonDelegate = InButtonDelegate;
}

void FSoldierMessageMenu::SetCancelClickedDelegate(FMessageMenuButtonClicked InButtonDelegate)
{
	CancelButtonDelegate = InButtonDelegate;
}


#undef LOCTEXT_NAMESPACE
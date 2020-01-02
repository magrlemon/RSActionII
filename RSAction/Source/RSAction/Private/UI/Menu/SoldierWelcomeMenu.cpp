// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "RSAction.h"
#include "SoldierWelcomeMenu.h"
#include "SoldierStyle.h"
#include "SSoldierConfirmationDialog.h"
#include "RSActionGameViewportClient.h"
#include "RSActionGameInstance.h"
#include "OnlineSubsystemUtils.h"

#define LOCTEXT_NAMESPACE "SoldierGame.HUD.Menu"

class SSoldierWelcomeMenuWidget : public SCompoundWidget
{
	/** The menu that owns this widget. */
	FSoldierWelcomeMenu* MenuOwner;

	/** Animate the text so that the screen isn't static, for console cert requirements. */
	FCurveSequence TextAnimation;

	/** The actual curve that animates the text. */
	FCurveHandle TextColorCurve;

	TSharedPtr<SRichTextBlock> PressPlayText;

	SLATE_BEGIN_ARGS( SSoldierWelcomeMenuWidget )
	{}

	SLATE_ARGUMENT(FSoldierWelcomeMenu*, MenuOwner)

	SLATE_END_ARGS()

	virtual bool SupportsKeyboardFocus() const override
	{
		return true;
	}

	UWorld* GetWorld() const
	{
		if (MenuOwner && MenuOwner->GetGameInstance().IsValid())
		{
			return MenuOwner->GetGameInstance()->GetWorld();
		}

		return nullptr;
	}

	void Construct( const FArguments& InArgs )
	{
		MenuOwner = InArgs._MenuOwner;
		
		TextAnimation = FCurveSequence();
		const float AnimDuration = 1.5f;
		TextColorCurve = TextAnimation.AddCurve(0, AnimDuration, ECurveEaseFunction::QuadInOut);

		ChildSlot
		[
			SNew(SBorder)
			.Padding(30.0f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[ 
				SAssignNew( PressPlayText, SRichTextBlock )
#if PLATFORM_PS4
				.Text( LOCTEXT("PressStartPS4", "PRESS CROSS BUTTON TO PLAY" ) )
#elif PLATFORM_SWITCH
				.Text(LOCTEXT("PressStartSwitch", "PRESS <img src=\"SoldierGame.Switch.Right\"/> TO PLAY"))
#else
				.Text( LOCTEXT("PressStartXboxOne", "PRESS A TO PLAY" ) )
#endif
				.TextStyle( FSoldierStyle::Get(), "SoldierGame.WelcomeScreen.WelcomeTextStyle" )
				.DecoratorStyleSet(&FSoldierStyle::Get())
				+ SRichTextBlock::ImageDecorator()
			]
		];
	}

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override
	{
		if(!TextAnimation.IsPlaying())
		{
			if(TextAnimation.IsAtEnd())
			{
				TextAnimation.PlayReverse(this->AsShared());
			}
			else
			{
				TextAnimation.Play(this->AsShared());
			}
		}

		PressPlayText->SetRenderOpacity(FMath::Lerp(0.5f, 1.0f, TextColorCurve.GetLerp()));
	}

	virtual FReply OnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override
	{
		return FReply::Handled();
	}

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
	{
		const FKey Key = InKeyEvent.GetKey();
		if (Key == EKeys::Enter)
		{
			TSharedPtr<const FUniqueNetId> UserId;
			const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
			if (OnlineSub)
			{
				const IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
				if (IdentityInterface.IsValid())
				{
					UserId = IdentityInterface->GetUniquePlayerId(InKeyEvent.GetUserIndex());
				}
			}
			MenuOwner->HandleLoginUIClosed(UserId, InKeyEvent.GetUserIndex());
		}
		else if (!MenuOwner->GetControlsLocked() && Key == EKeys::Virtual_Accept)
		{
			bool bSkipToMainMenu = true;

			{
				const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
				if (OnlineSub)
				{
					const IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
					if (IdentityInterface.IsValid())
					{
						TSharedPtr<GenericApplication> GenericApplication = FSlateApplication::Get().GetPlatformApplication();
						const bool bIsLicensed = GenericApplication->ApplicationLicenseValid();

						const ELoginStatus::Type LoginStatus = IdentityInterface->GetLoginStatus(InKeyEvent.GetUserIndex());
						if (LoginStatus == ELoginStatus::NotLoggedIn || !bIsLicensed)
						{
							// Show the account picker.
							const IOnlineExternalUIPtr ExternalUI = OnlineSub->GetExternalUIInterface();
							if (ExternalUI.IsValid())
							{
								ExternalUI->ShowLoginUI(InKeyEvent.GetUserIndex(), false, true, FOnLoginUIClosedDelegate::CreateSP(MenuOwner, &FSoldierWelcomeMenu::HandleLoginUIClosed));
								bSkipToMainMenu = false;
							}
						}
					}
				}
			}

			if (bSkipToMainMenu)
			{
				const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
				if (OnlineSub)
				{
					const IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
					if (IdentityInterface.IsValid())
					{
						TSharedPtr<const FUniqueNetId> UserId = IdentityInterface->GetUniquePlayerId(InKeyEvent.GetUserIndex());
						// If we couldn't show the external login UI for any reason, or if the user is
						// already logged in, just advance to the main menu immediately.
						MenuOwner->HandleLoginUIClosed(UserId, InKeyEvent.GetUserIndex());
					}
				}
			}

			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

	virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override
	{
		return FReply::Handled().ReleaseMouseCapture().SetUserFocus(SharedThis( this ), EFocusCause::SetDirectly, true);
	}
};

void FSoldierWelcomeMenu::Construct( TWeakObjectPtr< URSActionGameInstance > InGameInstance )
{
	bControlsLocked = false;
	GameInstance = InGameInstance;
	PendingControllerIndex = -1;

	MenuWidget = SNew( SSoldierWelcomeMenuWidget )
		.MenuOwner(this);	
}

void FSoldierWelcomeMenu::AddToGameViewport()
{
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->AddViewportWidgetContent(MenuWidget.ToSharedRef());
		FSlateApplication::Get().SetKeyboardFocus(MenuWidget);
	}
}

void FSoldierWelcomeMenu::RemoveFromGameViewport()
{
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(MenuWidget.ToSharedRef());
	}
}

void FSoldierWelcomeMenu::HandleLoginUIClosed(TSharedPtr<const FUniqueNetId> UniqueId, const int ControllerIndex, const FOnlineError& Error)
{
	if ( !ensure( GameInstance.IsValid() ) )
	{
		return;
	}

	USoldierGameViewportClient* SoldierViewport = Cast<USoldierGameViewportClient>( GameInstance->GetGameViewportClient() );

	TSharedPtr<GenericApplication> GenericApplication = FSlateApplication::Get().GetPlatformApplication();
	const bool bIsLicensed = GenericApplication->ApplicationLicenseValid();

	// If they don't currently have a license, let them know, but don't let them proceed
	if (!bIsLicensed && SoldierViewport != NULL)
	{
		const FText StopReason	= NSLOCTEXT( "ProfileMessages", "NeedLicense", "The signed in users do not have a license for this game. Please purchase SoldierGame from the Xbox Marketplace or sign in a user with a valid license." );
		const FText OKButton	= NSLOCTEXT( "DialogButtons", "OKAY", "OK" );

		SoldierViewport->ShowDialog( 
			nullptr,
			ESoldierDialogType::Generic,
			StopReason,
			OKButton,
			FText::GetEmpty(),
			FOnClicked::CreateRaw(this, &FSoldierWelcomeMenu::OnConfirmGeneric),
			FOnClicked::CreateRaw(this, &FSoldierWelcomeMenu::OnConfirmGeneric)
			);
		return;
	}

	PendingControllerIndex = ControllerIndex;

	if (UniqueId.IsValid())
	{
		// Next step, check privileges
		const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GameInstance->GetWorld());
		if (OnlineSub)
		{
			const IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
			if (IdentityInterface.IsValid())
			{
				IdentityInterface->GetUserPrivilege(*UniqueId, EUserPrivileges::CanPlay, IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateSP(this, &FSoldierWelcomeMenu::OnUserCanPlay));
			}
		}
	}
	else
	{
		// Show a warning that your progress won't be saved if you continue without logging in. 
		if (SoldierViewport != NULL)
		{
			SoldierViewport->ShowDialog( 
				nullptr,
				ESoldierDialogType::Generic,
				NSLOCTEXT("ProfileMessages", "ProgressWillNotBeSaved", "If you continue without signing in, your progress will not be saved."),
				NSLOCTEXT("DialogButtons", "AContinue", "A - Continue"),
				NSLOCTEXT("DialogButtons", "BBack", "B - Back"),
				FOnClicked::CreateRaw(this, &FSoldierWelcomeMenu::OnContinueWithoutSavingConfirm),
				FOnClicked::CreateRaw(this, &FSoldierWelcomeMenu::OnConfirmGeneric)
			);
		}
	}
}

void FSoldierWelcomeMenu::SetControllerAndAdvanceToMainMenu(const int ControllerIndex)
{
	if ( !ensure( GameInstance.IsValid() ) )
	{
		return;
	}

	ULocalPlayer * NewPlayerOwner = GameInstance->GetFirstGamePlayer();

	if ( NewPlayerOwner != nullptr && ControllerIndex != -1 )
	{
		NewPlayerOwner->SetControllerId(ControllerIndex);
		NewPlayerOwner->SetCachedUniqueNetId(NewPlayerOwner->GetUniqueNetIdFromCachedControllerId().GetUniqueNetId());

		// tell gameinstance to transition to main menu
		GameInstance->GotoState(RSActionGameInstanceState::MainMenu);
	}	
}

FReply FSoldierWelcomeMenu::OnContinueWithoutSavingConfirm()
{
	if ( !ensure( GameInstance.IsValid() ) )
	{
		return FReply::Handled();
	}

	USoldierGameViewportClient * SoldierViewport = Cast<USoldierGameViewportClient>( GameInstance->GetGameViewportClient() );

	if (SoldierViewport != NULL)
	{
		SoldierViewport->HideDialog();
	}

	SetControllerAndAdvanceToMainMenu(PendingControllerIndex);
	return FReply::Handled();
}

FReply FSoldierWelcomeMenu::OnConfirmGeneric()
{
	if ( !ensure( GameInstance.IsValid() ) )
	{
		return FReply::Handled();
	}

	USoldierGameViewportClient * SoldierViewport = Cast<USoldierGameViewportClient>( GameInstance->GetGameViewportClient() );

	if (SoldierViewport != NULL)
	{
		SoldierViewport->HideDialog();
	}

	return FReply::Handled();
}

void FSoldierWelcomeMenu::OnUserCanPlay(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults)
{
	if (PrivilegeResults == (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures)
	{
		SetControllerAndAdvanceToMainMenu(PendingControllerIndex);
	}
	else
	{
		USoldierGameViewportClient * SoldierViewport = Cast<USoldierGameViewportClient>( GameInstance->GetGameViewportClient() );

		if ( SoldierViewport != NULL )
		{
			const FText ReturnReason = NSLOCTEXT("PrivilegeFailures", "CannotPlayAgeRestriction", "You cannot play this game due to age restrictions.");
			const FText OKButton = NSLOCTEXT("DialogButtons", "OKAY", "OK");

			SoldierViewport->ShowDialog( 
				nullptr,
				ESoldierDialogType::Generic,
				ReturnReason,
				OKButton,
				FText::GetEmpty(),
				FOnClicked::CreateRaw(this, &FSoldierWelcomeMenu::OnConfirmGeneric),
				FOnClicked::CreateRaw(this, &FSoldierWelcomeMenu::OnConfirmGeneric)
			);
		}
	}
}

#undef LOCTEXT_NAMESPACE

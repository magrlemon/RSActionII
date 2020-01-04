// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SoldierGame.h"
#include "SoldierStyle.h"
#include "SSoldierSplitScreenLobbyWidget.h"
#include "SoldierMenuItemWidgetStyle.h"
#include "SoldierMenuWidgetStyle.h"
#include "SSoldierConfirmationDialog.h"
#include "SoldierGameViewportClient.h"
#include "OnlineSubsystemUtils.h"

#define LOCTEXT_NAMESPACE "SoldierGame.SplitScreenLobby"

int32 GSoldierSplitScreenMax = 2;
static FAutoConsoleVariableRef CVarSoldierSplitScreenMax(
	TEXT("r.SoldierSplitScreenMax"),
	GSoldierSplitScreenMax,
	TEXT("Maximum number of split screen players.\n")
	TEXT("This will set the number of slots available in the split screen lobby.\n")
	TEXT("Default is 2."),
	ECVF_Default
	);

void SSoldierSplitScreenLobby::Construct( const FArguments& InArgs )
{
#if PLATFORM_PS4
	PressToPlayText = LOCTEXT("PressToPlay", "Press cross button to Play");
	PressToFindText = LOCTEXT("PressToFind", "Press cross button to Find Match");
	PressToStartMatchText = LOCTEXT("PressToStart", "Press cross button To Start Match");
#else
	PressToPlayText = LOCTEXT("PressToPlay", "Press A to Play");
	PressToFindText = LOCTEXT("PressToFind", "Press A to Find Match");
	PressToStartMatchText = LOCTEXT("PressToStart", "Press A To Start Match");	
#endif

#if PLATFORM_SWITCH
	PressToPlayText = LOCTEXT("PressToPlay", "<img src=\"SoldierGame.Switch.Right\"/> Select User");
	PressToFindText = LOCTEXT("PressToFind", "Press <img src=\"SoldierGame.Switch.Right\"/> to Find Match");
	PressToStartMatchText = LOCTEXT("PressToStart", "<img src=\"SoldierGame.Switch.Right\"/> Start Match / <img src=\"SoldierGame.Switch.Up\"/> Connect Controllers");
	PlayAsGuestText = LOCTEXT("PlayAsGuest", "<img src=\"SoldierGame.Switch.Left\"/> Play As Guest");
#endif	

	PlayerOwner = InArgs._PlayerOwner;

	bIsJoining = false;

	const FSoldierMenuStyle* ItemStyle = &FSoldierStyle::Get().GetWidgetStyle<FSoldierMenuStyle>("DefaultSoldierMenuStyle");
	const FSlateBrush* SlotBrush = &ItemStyle->LeftBackgroundBrush;

	const FButtonStyle* ButtonStyle = &FSoldierStyle::Get().GetWidgetStyle<FButtonStyle>("DefaultSoldierButtonStyle");
	FLinearColor MenuTitleTextColor =  FLinearColor(FColor(155,164,182));
	FLinearColor PressXToStartColor = FLinearColor::Green;
	
	MasterUserBack = InArgs._OnCancelClicked;
	MasterUserPlay = InArgs._OnPlayClicked;	

	const float PaddingBetweenSlots = 10.0f;
	const float SlotWidth = 565.0f;
	const float SlotHeight = 300.0f;

	const FText Msg = LOCTEXT("You must be signed in to play online", "You must be signed in to play online");
	const FText OKButtonString = NSLOCTEXT("DialogButtons", "OKAY", "OK");

	ChildSlot
	[	
		//main container.  Just start us out centered on the whole screen.
		SNew(SOverlay)
		+SOverlay::Slot()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		[
			//at maximum we need a 2x2 grid.  So we need two vertical slots, which will each contain 2 horizontal slots.
			SNew( SVerticalBox )
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(PaddingBetweenSlots) //put some space in between the slots
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew( SHorizontalBox)
				+SHorizontalBox::Slot()				
				.Padding(PaddingBetweenSlots) //put some space in between the slots				
				[
					SAssignNew(UserSlots[0], SBox)
					.HeightOverride(SlotHeight)
					.WidthOverride(SlotWidth)
					[
						SNew(SBorder)
						.Padding(0.0f)						
						.BorderImage(SlotBrush)
						.BorderBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()												
							.Padding(0.0f)
							.VAlign(VAlign_Bottom)
							.HAlign(HAlign_Center)
							[
								//first slot needs to have some text to say 'press X to start match'. Only master user can start the match.
								SAssignNew(UserTextWidgets[0], SRichTextBlock)
								.TextStyle(FSoldierStyle::Get(), "SoldierGame.MenuHeaderTextStyle")
//								.ColorAndOpacity(MenuTitleTextColor)
								.Text(PressToPlayText)														
								.DecoratorStyleSet(&FSoldierStyle::Get())
								+ SRichTextBlock::ImageDecorator()
							]
							+SVerticalBox::Slot()							
							.Padding(5.0f)
							.VAlign(VAlign_Bottom)
							.HAlign(HAlign_Center)					
							[							
								SNew(SRichTextBlock)
								.TextStyle(FSoldierStyle::Get(), "SoldierGame.SplitScreenLobby.StartMatchTextStyle")							
								.Text(this, &SSoldierSplitScreenLobby::GetPlayFindText)
								.DecoratorStyleSet(&FSoldierStyle::Get())
								+ SRichTextBlock::ImageDecorator()
							]
						]					
					]
				]
				+SHorizontalBox::Slot()					
				.Padding(PaddingBetweenSlots)				
				[
					SAssignNew(UserSlots[1], SBox)
					.HeightOverride(SlotHeight)
					.WidthOverride(SlotWidth)
					[
#if PLATFORM_SWITCH
						SNew(SBorder)
						.Padding(0.0f)						
						.BorderImage(SlotBrush)
						.BorderBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()												
							.Padding(0.0f)
							.VAlign(VAlign_Bottom)
							.HAlign(HAlign_Center)
							[
								//first slot needs to have some text to say 'press X to start match'. Only master user can start the match.
								SAssignNew(UserTextWidgets[1], SRichTextBlock)
								.TextStyle(FSoldierStyle::Get(), "SoldierGame.MenuHeaderTextStyle")
								//.ColorAndOpacity(MenuTitleTextColor)
								.Text(PressToPlayText)														
								.DecoratorStyleSet(&FSoldierStyle::Get())
								+ SRichTextBlock::ImageDecorator()
							]
							+SVerticalBox::Slot()							
							.Padding(5.0f)
							.VAlign(VAlign_Bottom)
							.HAlign(HAlign_Center)					
							[							
								SNew(SRichTextBlock)
								.TextStyle(FSoldierStyle::Get(), "SoldierGame.SplitScreenLobby.StartMatchTextStyle")							
								.Text(this, &SSoldierSplitScreenLobby::GetPlayAsGuestText)
								.DecoratorStyleSet(&FSoldierStyle::Get())
								+ SRichTextBlock::ImageDecorator()
							]
						]	
#else
						SNew(SBorder)
						.Padding(0.0f)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						.BorderImage(SlotBrush)
						.BorderBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
						[
							SAssignNew(UserTextWidgets[1], SRichTextBlock)
							.TextStyle(FSoldierStyle::Get(), "SoldierGame.MenuHeaderTextStyle")
							//.ColorAndOpacity(MenuTitleTextColor)
							.Text(PressToPlayText)
							.DecoratorStyleSet(&FSoldierStyle::Get())
							+ SRichTextBlock::ImageDecorator()
						]
#endif
					]
				]
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.Padding(PaddingBetweenSlots)
			[
				SNew( SHorizontalBox)
				+SHorizontalBox::Slot()							
				.Padding(PaddingBetweenSlots)				
				[
					SAssignNew(UserSlots[2], SBox)
					.HeightOverride(SlotHeight)
					.WidthOverride(SlotWidth)
					[
						SNew(SBorder)
						.Padding(0.0f)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						.BorderImage(SlotBrush)
						.BorderBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
						[
							SAssignNew(UserTextWidgets[2], SRichTextBlock)
							.TextStyle(FSoldierStyle::Get(), "SoldierGame.MenuHeaderTextStyle")
							//.ColorAndOpacity(MenuTitleTextColor)
							.Text(PressToPlayText)
							.DecoratorStyleSet(&FSoldierStyle::Get())
							+ SRichTextBlock::ImageDecorator()
						]
					]
				]

				+SHorizontalBox::Slot()				
				.Padding(PaddingBetweenSlots)				
				[
					SAssignNew(UserSlots[3], SBox)
					.HeightOverride(SlotHeight)
					.WidthOverride(SlotWidth)
					[
						SNew(SBorder)
						.Padding(0.0f)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						.BorderImage(SlotBrush)
						.BorderBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
						[
							SAssignNew(UserTextWidgets[3], SRichTextBlock)
							.TextStyle(FSoldierStyle::Get(), "SoldierGame.MenuHeaderTextStyle")
							//.ColorAndOpacity(MenuTitleTextColor)
							.Text(PressToPlayText)
							.DecoratorStyleSet(&FSoldierStyle::Get())
							+ SRichTextBlock::ImageDecorator()
						]
					]
				]	
			]			
		]
	];
	
	Clear();
}

void SSoldierSplitScreenLobby::Clear()
{	
	if (GetGameInstance() == nullptr)
	{
		return;
	}

	// Remove existing splitscreen players
	GetGameInstance()->RemoveSplitScreenPlayers();

	// Sync the list with the actively tracked local users
	UpdateSlots();
}

void SSoldierSplitScreenLobby::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	// Parent tick
	SCompoundWidget::Tick( AllottedGeometry, InCurrentTime, InDeltaTime);

	// Make sure the slots match the local user list
	UpdateSlots();
}

void SSoldierSplitScreenLobby::UpdateSlots()
{
	if (GetGameInstance() == nullptr)
	{
		return;
	}

	// Show active players
	for ( int i = 0; i < GetNumSupportedSlots(); ++i )
	{
		ULocalPlayer * LocalPlayer = ( i < GetGameInstance()->GetNumLocalPlayers() ) ? GetGameInstance()->GetLocalPlayerByIndex( i ) : NULL;

		if ( LocalPlayer != NULL )
		{
			UserTextWidgets[i]->SetText( FText::FromString(LocalPlayer->GetNickname()) );
		}
		else
		{ 
			UserTextWidgets[i]->SetText( PressToPlayText );
		}

		UserSlots[i]->SetVisibility( EVisibility::Visible );
	}
	
	// Hide non supported slots
	for ( int i = GetNumSupportedSlots(); i < MAX_POSSIBLE_SLOTS; ++i )
	{
		UserSlots[i]->SetVisibility( EVisibility::Collapsed );
	}
}

void SSoldierSplitScreenLobby::ConditionallyReadyPlayer( const int ControllerId, const bool bCanShowUI )
{
	USoldierGameInstance* const GameInstance = GetGameInstance();
	if (GameInstance == nullptr)
	{
		return;
	}
	
	if (GameInstance->GetNumLocalPlayers() >= GetNumSupportedSlots() )
	{
		return;		// Can't fit any more players at this point
	}

	// If we already have this controller id registered, ignore this request
	if (GameInstance->FindLocalPlayerFromControllerId( ControllerId ) != NULL )
	{
		return;
	}

	TSharedPtr< const FUniqueNetId > UniqueNetId = GameInstance->GetUniqueNetIdFromControllerId( ControllerId );

	const bool bIsPlayerOnline		= ( UniqueNetId.IsValid() && IsUniqueIdOnline( *UniqueNetId ) );
	const bool bFoundExistingPlayer = GameInstance->FindLocalPlayerFromUniqueNetId( UniqueNetId ) != nullptr;
	const EOnlineMode OnlineMode = GameInstance->GetOnlineMode();
	// If this is an online game, reject and show sign-in if:
	//	1. We have already registered this FUniqueNetId
	//	2. This player is not currently signed in and online
	// on Swtich, always show the login UI even if a Lan match
	if (
#if PLATFORM_SWITCH
		bCanShowUI &&
#else
		OnlineMode == EOnlineMode::Online &&
#endif
		( bFoundExistingPlayer || !bIsPlayerOnline ) )
	{
		const IOnlineExternalUIPtr ExternalUI = Online::GetExternalUIInterface(GameInstance->GetWorld());

		if ( bCanShowUI && ExternalUI.IsValid() )
		{
			ExternalUI->ShowLoginUI( ControllerId, OnlineMode == EOnlineMode::Online, false, FOnLoginUIClosedDelegate::CreateSP( this, &SSoldierSplitScreenLobby::HandleLoginUIClosedAndReady ) );
		}

		return;
	}

	if (bIsPlayerOnline)
	{
		const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GameInstance->GetWorld());
		if (OnlineSub)
		{
			const IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
			if(Identity.IsValid())
			{
				PendingControllerId = ControllerId;
				Identity->GetUserPrivilege(
					*UniqueNetId,
					OnlineMode != EOnlineMode::Offline ? EUserPrivileges::CanPlayOnline : EUserPrivileges::CanPlay,
					IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateSP(this, &SSoldierSplitScreenLobby::OnUserCanPlay));
			}
		}
	}
	else
	{
		ReadyPlayer(ControllerId);
	}
}

void SSoldierSplitScreenLobby::ReadyPlayer( const int ControllerId )
{
	FString Error;
	ULocalPlayer * LocalPlayer = GetGameInstance()->CreateLocalPlayer(ControllerId, Error, false);

	if (LocalPlayer != NULL)
	{
		// We have UserId, but we want to make sure we're using the same shared pointer from the game instance so all the reference counting works out
		TSharedPtr< const FUniqueNetId > UniqueNetId = GetGameInstance()->GetUniqueNetIdFromControllerId(ControllerId);
		LocalPlayer->SetCachedUniqueNetId(UniqueNetId);
	}
}

void SSoldierSplitScreenLobby::UnreadyPlayer( const int ControllerId )
{
	if (GetGameInstance() == nullptr)
	{
		return;
	}

	ULocalPlayer * LocalPlayer = GEngine->GetLocalPlayerFromControllerId( GetGameInstance()->GetGameViewportClient(), ControllerId );

	if ( LocalPlayer == NULL )
	{
		UE_LOG( LogOnline, Warning, TEXT( "SSoldierSplitScreenLobby::UnreadyPlayer: ControllerId not found: %i" ), ControllerId );
		return;
	}

	GetGameInstance()->RemoveExistingLocalPlayer( LocalPlayer );
}

FReply SSoldierSplitScreenLobby::OnOkOrCancel()
{
	USoldierGameViewportClient* SoldierViewport = Cast<USoldierGameViewportClient>(GetGameInstance()->GetGameViewportClient());

	if (SoldierViewport != NULL)
	{
		SoldierViewport->HideDialog();
	}

	return FReply::Handled();
}

bool SSoldierSplitScreenLobby::ConfirmSponsorsSatisfied() const
{
	if (GetGameInstance() == nullptr)
	{
		return false;
	}

	const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetGameInstance()->GetWorld());

	if(OnlineSub)
	{
		const IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
		if(IdentityInterface.IsValid())
		{
			const int32 NumActiveSlots = FMath::Min( GetNumSupportedSlots(), GetGameInstance()->GetNumLocalPlayers() );

			for ( int i = 0; i < NumActiveSlots; ++i )
			{
				ULocalPlayer * LocalPlayer = GetGameInstance()->GetLocalPlayerByIndex( i );

				TSharedPtr<const FUniqueNetId> Sponsor = IdentityInterface->GetSponsorUniquePlayerId( LocalPlayer->GetControllerId() );

				// If this user has a sponsor (is a guest), make sure our sponsor is playing with us
				if(Sponsor.IsValid())
				{
					return ( GetGameInstance()->FindLocalPlayerFromUniqueNetId( Sponsor ) != NULL );
				}
			}
		}
	}

	return true;
}

void SSoldierSplitScreenLobby::OnUserCanPlay(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults)
{
	if (PrivilegeResults != (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures && GetGameInstance())
	{
		// Xbox shows its own system dialog currently
#if PLATFORM_PS4
		const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetGameInstance()->GetWorld());
		if (OnlineSub)
		{
			const IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
			if (Identity.IsValid())
			{
				FString Nickname = Identity->GetPlayerNickname(UserId);

				// Show warning that the user cannot play due to age restrictions
				USoldierGameViewportClient * SoldierViewport = Cast<USoldierGameViewportClient>(GetGameInstance()->GetGameViewportClient());

				if (SoldierViewport != NULL)
				{
					SoldierViewport->ShowDialog(
						PlayerOwner.Get(),
						ESoldierDialogType::Generic,
						FText::Format(NSLOCTEXT("ProfileMessages", "AgeRestrictionFmt", "{0} cannot play due to age restrictions!"), FText::FromString(Nickname)),
						NSLOCTEXT("DialogButtons", "OKAY", "OK"),
						FText::GetEmpty(),
						FOnClicked::CreateRaw(this, &SSoldierSplitScreenLobby::OnOkOrCancel),
						FOnClicked::CreateRaw(this, &SSoldierSplitScreenLobby::OnOkOrCancel)
						);
				}
			}
		}
#endif
	}
	else
	{
		ReadyPlayer(PendingControllerId);
	}
}

int32 SSoldierSplitScreenLobby::GetNumSupportedSlots() const
{
	return FMath::Clamp( GSoldierSplitScreenMax, 1, MAX_POSSIBLE_SLOTS );
}

bool SSoldierSplitScreenLobby::IsUniqueIdOnline( const FUniqueNetId& UniqueId ) const
{
	if (GetGameInstance() == nullptr)
	{
		return false;
	}

	IOnlineIdentityPtr Identity = Online::GetIdentityInterface(GetGameInstance()->GetWorld());

	if ( !Identity.IsValid() )
	{
		return false;
	}

	return Identity->GetLoginStatus( UniqueId ) == ELoginStatus::LoggedIn;
}

FReply SSoldierSplitScreenLobby::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const USoldierGameInstance* GameInstance = GetGameInstance();
	if (GameInstance == nullptr)
	{
		return FReply::Unhandled();
	}

	const FKey Key = InKeyEvent.GetKey();
	const int32 UserIndex = InKeyEvent.GetUserIndex();

	ULocalPlayer * ExistingPlayer = GEngine->GetLocalPlayerFromControllerId(GetGameInstance()->GetGameViewportClient(), UserIndex);

	if ((Key == EKeys::Enter || Key == EKeys::Virtual_Accept) && !InKeyEvent.IsRepeat())
	{
		// See if we are already tracking this user
		if ( ExistingPlayer != NULL )
		{
			// See if this is the initial user
			if ( ExistingPlayer == GameInstance->GetFirstGamePlayer() )
			{
				// If this is the initial user, start the game
				if (GameInstance->GetOnlineMode() != EOnlineMode::Online || ConfirmSponsorsSatisfied() )
				{
					return MasterUserPlay.Execute();
				}
				else
				{
					// Show warning that the guest needs the sponsor
					USoldierGameViewportClient * SoldierViewport = Cast<USoldierGameViewportClient>(GameInstance->GetGameViewportClient());

					if ( SoldierViewport != NULL )
					{
						SoldierViewport->ShowDialog( 
							PlayerOwner.Get(),
							ESoldierDialogType::Generic,
							NSLOCTEXT("ProfileMessages", "GuestAccountNeedsSponsor", "A guest account cannot play without its sponsor!"),
							NSLOCTEXT("DialogButtons", "OKAY", "OK"),
							FText::GetEmpty(),
							FOnClicked::CreateRaw(this, &SSoldierSplitScreenLobby::OnOkOrCancel),
							FOnClicked::CreateRaw(this, &SSoldierSplitScreenLobby::OnOkOrCancel)
						);
					}

					return FReply::Handled();
				}
			}
	
			return FReply::Handled();
		}

		ConditionallyReadyPlayer(UserIndex, true);

		return FReply::Handled();
	}
#if PLATFORM_SWITCH
	else if ((Key == EKeys::Gamepad_FaceButton_Top) && !InKeyEvent.IsRepeat())
	{
		GEngine->DeferredCommands.Add(TEXT("Npad.ConnectUI"));
	}
	else if (GameInstance->GetOnlineMode() != EOnlineMode::Online && (Key == EKeys::Gamepad_FaceButton_Left) && !InKeyEvent.IsRepeat())
	{
		ConditionallyReadyPlayer(UserIndex, false);
	}
#endif
	else if (Key == EKeys::Escape || Key == EKeys::Virtual_Back || Key == EKeys::Global_Back)
	{
		if ( ExistingPlayer != NULL && ExistingPlayer == GetGameInstance()->GetFirstGamePlayer() )
		{
			return MasterUserBack.Execute();
		}
		else
		{
			UnreadyPlayer(UserIndex);
		}
	}

	return FReply::Unhandled();
}

FReply SSoldierSplitScreenLobby::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
{
	// focus all possible users
	for (int Index = 0; Index < GSoldierSplitScreenMax; Index++)
	{
		FSlateApplication::Get().SetUserFocus(Index, SharedThis(this), EFocusCause::SetDirectly);
	}
	return FReply::Handled().ReleaseMouseCapture();

}

void SSoldierSplitScreenLobby::OnFocusLost( const FFocusEvent& InFocusEvent )
{
}

void SSoldierSplitScreenLobby::HandleLoginUIClosedAndReady( TSharedPtr<const FUniqueNetId> UniqueId, const int UserIndex, const FOnlineError& Error )
{
	const USoldierGameInstance* GameInstance = GetGameInstance();
	if (GameInstance == nullptr)
	{
		return;
	}

	EOnlineMode OnlineMode = GameInstance->GetOnlineMode();

	// If a player signed in, UniqueId will be valid, and we can place him in the open slot.
	if ( UniqueId.IsValid() )
	{
		if ( OnlineMode != EOnlineMode::Online || IsUniqueIdOnline( *UniqueId ) )
		{
			ConditionallyReadyPlayer( UserIndex, false );
		}
		else if (!IsUniqueIdOnline(*UniqueId))
		{
			if (OnlineMode == EOnlineMode::Online)
			{
				const UWorld* World = GetGameInstance()->GetWorld();

				OnLoginCompleteDelegateHandle = Online::GetIdentityInterface(World)->AddOnLoginCompleteDelegate_Handle(UserIndex, FOnLoginCompleteDelegate::CreateRaw(this, &SSoldierSplitScreenLobby::OnLoginComplete));
				Online::GetIdentityInterface(World)->Login(UserIndex, FOnlineAccountCredentials());
			}
			else
			{
				ConditionallyReadyPlayer(UserIndex, false);
			}
		}
	}
}

void SSoldierSplitScreenLobby::OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	Online::GetIdentityInterface(GetGameInstance()->GetWorld())->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, OnLoginCompleteDelegateHandle);

	if (bWasSuccessful && IsUniqueIdOnline(*GetGameInstance()->GetUniqueNetIdFromControllerId(LocalUserNum)))
	{
		ConditionallyReadyPlayer(LocalUserNum, false);
	}
}

USoldierGameInstance * SSoldierSplitScreenLobby::GetGameInstance() const
{
	if ( !PlayerOwner.IsValid() )
	{
		return NULL;
	}

	check( PlayerOwner->GetGameInstance() == nullptr || CastChecked< USoldierGameInstance >( PlayerOwner->GetGameInstance() ) != nullptr );

	return Cast< USoldierGameInstance >( PlayerOwner->GetGameInstance() );
}

FText SSoldierSplitScreenLobby::GetPlayFindText() const
{
	return bIsJoining ? PressToFindText : PressToStartMatchText;
}

#if PLATFORM_SWITCH
FText SSoldierSplitScreenLobby::GetPlayAsGuestText() const
{
	const USoldierGameInstance* GameInstance = GetGameInstance();
	if (GameInstance == nullptr)
	{
		return FText();
	}

	return GameInstance->GetOnlineMode() != EOnlineMode::Offline ? FText() : PlayAsGuestText;
}
#endif

#undef LOCTEXT_NAMESPACE

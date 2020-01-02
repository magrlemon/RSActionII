// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SoldierGameInstance.cpp
=============================================================================*/

#include "SoldierGame.h"
#include "SoldierGameInstance.h"
#include "SoldierMainMenu.h"
#include "SoldierWelcomeMenu.h"
#include "SoldierMessageMenu.h"
#include "SoldierGameLoadingScreen.h"
#include "OnlineKeyValuePair.h"
#include "SoldierStyle.h"
#include "SoldierMenuItemWidgetStyle.h"
#include "SoldierGameViewportClient.h"
#include "Player/SoldierPlayerController_Menu.h"
#include "Online/SoldierPlayerState.h"
#include "Online/SoldierGameSession.h"
#include "Online/SoldierOnlineSessionClient.h"
#include "OnlineSubsystemUtils.h"

FAutoConsoleVariable CVarSoldierGameTestEncryption(TEXT("SoldierGame.TestEncryption"), 0, TEXT("If true, clients will send an encryption token with their request to join the server and attempt to encrypt the connection using a debug key. This is NOT SECURE and for demonstration purposes only."));

void SSoldierWaitDialog::Construct(const FArguments& InArgs)
{
	const FSoldierMenuItemStyle* ItemStyle = &FSoldierStyle::Get().GetWidgetStyle<FSoldierMenuItemStyle>("DefaultSoldierMenuItemStyle");
	const FButtonStyle* ButtonStyle = &FSoldierStyle::Get().GetWidgetStyle<FButtonStyle>("DefaultSoldierButtonStyle");
	ChildSlot
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
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
					SNew(STextBlock)
					.TextStyle(FSoldierStyle::Get(), "SoldierGame.MenuHeaderTextStyle")
					.ColorAndOpacity(this, &SSoldierWaitDialog::GetTextColor)
					.Text(InArgs._MessageText)
					.WrapTextAt(500.0f)
				]
			]
		];

	//Setup a curve
	const float StartDelay = 0.0f;
	const float SecondDelay = 0.0f;
	const float AnimDuration = 2.0f;

	WidgetAnimation = FCurveSequence();
	TextColorCurve = WidgetAnimation.AddCurve(StartDelay + SecondDelay, AnimDuration, ECurveEaseFunction::QuadInOut);
	WidgetAnimation.Play(this->AsShared(), true);
}

FSlateColor SSoldierWaitDialog::GetTextColor() const
{
	//instead of going from black -> white, go from white -> grey.
	float fAlpha = 1.0f - TextColorCurve.GetLerp();
	fAlpha = fAlpha * 0.5f + 0.5f;
	return FLinearColor(FColor(155, 164, 182, FMath::Clamp((int32)(fAlpha * 255.0f), 0, 255)));
}

namespace SoldierGameInstanceState
{
	const FName None = FName(TEXT("None"));
	const FName PendingInvite = FName(TEXT("PendingInvite"));
	const FName WelcomeScreen = FName(TEXT("WelcomeScreen"));
	const FName MainMenu = FName(TEXT("MainMenu"));
	const FName MessageMenu = FName(TEXT("MessageMenu"));
	const FName Playing = FName(TEXT("Playing"));
}


USoldierGameInstance::USoldierGameInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, OnlineMode(EOnlineMode::Online) // Default to online
	, bIsLicensed(true) // Default to licensed (should have been checked by OS on boot)
{
	CurrentState = SoldierGameInstanceState::None;
}

void USoldierGameInstance::Init() 
{
	Super::Init();

	IgnorePairingChangeForControllerId = -1;
	CurrentConnectionStatus = EOnlineServerConnectionStatus::Connected;

	LocalPlayerOnlineStatus.InsertDefaulted(0, MAX_LOCAL_PLAYERS);

	// game requires the ability to ID users.
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	check(OnlineSub);
	const IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
	check(IdentityInterface.IsValid());

 	const IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
 	check(SessionInterface.IsValid());

	// bind any OSS delegates we needs to handle
	for (int i = 0; i < MAX_LOCAL_PLAYERS; ++i)
	{
		IdentityInterface->AddOnLoginStatusChangedDelegate_Handle(i, FOnLoginStatusChangedDelegate::CreateUObject(this, &USoldierGameInstance::HandleUserLoginChanged));
	}

	IdentityInterface->AddOnControllerPairingChangedDelegate_Handle(FOnControllerPairingChangedDelegate::CreateUObject(this, &USoldierGameInstance::HandleControllerPairingChanged));

	FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(this, &USoldierGameInstance::HandleAppWillDeactivate);

	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this, &USoldierGameInstance::HandleAppSuspend);
	FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(this, &USoldierGameInstance::HandleAppResume);

	FCoreDelegates::OnSafeFrameChangedEvent.AddUObject(this, &USoldierGameInstance::HandleSafeFrameChanged);
	FCoreDelegates::OnControllerConnectionChange.AddUObject(this, &USoldierGameInstance::HandleControllerConnectionChange);
	FCoreDelegates::ApplicationLicenseChange.AddUObject(this, &USoldierGameInstance::HandleAppLicenseUpdate);

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &USoldierGameInstance::OnPreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &USoldierGameInstance::OnPostLoadMap);

	FCoreUObjectDelegates::PostDemoPlay.AddUObject(this, &USoldierGameInstance::OnPostDemoPlay);

	bPendingEnableSplitscreen = false;

	OnlineSub->AddOnConnectionStatusChangedDelegate_Handle( FOnConnectionStatusChangedDelegate::CreateUObject( this, &USoldierGameInstance::HandleNetworkConnectionStatusChanged ) );

	if (SessionInterface.IsValid())
	{
		SessionInterface->AddOnSessionFailureDelegate_Handle( FOnSessionFailureDelegate::CreateUObject( this, &USoldierGameInstance::HandleSessionFailure ) );
	}
	
	OnEndSessionCompleteDelegate = FOnEndSessionCompleteDelegate::CreateUObject(this, &USoldierGameInstance::OnEndSessionComplete);

	// Register delegate for ticker callback
	TickDelegate = FTickerDelegate::CreateUObject(this, &USoldierGameInstance::Tick);
	TickDelegateHandle = FTicker::GetCoreTicker().AddTicker(TickDelegate);

	// Initialize the debug key with a set value for AES256. This is not secure and for example purposes only.
	DebugTestEncryptionKey.SetNum(32);

	for (int32 i = 0; i < DebugTestEncryptionKey.Num(); ++i)
	{
		DebugTestEncryptionKey[i] = uint8(i);
	}
}

void USoldierGameInstance::Shutdown()
{
	Super::Shutdown();

	// Unregister ticker delegate
	FTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
}

void USoldierGameInstance::HandleNetworkConnectionStatusChanged( const FString& ServiceName, EOnlineServerConnectionStatus::Type LastConnectionStatus, EOnlineServerConnectionStatus::Type ConnectionStatus )
{
	UE_LOG( LogOnlineGame, Log, TEXT( "USoldierGameInstance::HandleNetworkConnectionStatusChanged: %s" ), EOnlineServerConnectionStatus::ToString( ConnectionStatus ) );

#if SHOOTER_CONSOLE_UI
	// If we are disconnected from server, and not currently at (or heading to) the welcome screen
	// then display a message on consoles
	if (	OnlineMode != EOnlineMode::Offline && 
			PendingState != SoldierGameInstanceState::WelcomeScreen &&
			CurrentState != SoldierGameInstanceState::WelcomeScreen && 
			ConnectionStatus != EOnlineServerConnectionStatus::Connected &&
			ConnectionStatus != EOnlineServerConnectionStatus::Normal)
	{
		UE_LOG( LogOnlineGame, Log, TEXT( "USoldierGameInstance::HandleNetworkConnectionStatusChanged: Going to main menu" ) );

		// Display message on consoles
#if PLATFORM_XBOXONE
		const FText ReturnReason	= NSLOCTEXT( "NetworkFailures", "ServiceUnavailable", "Connection to Xbox LIVE has been lost." );
#elif PLATFORM_PS4
		const FText ReturnReason	= NSLOCTEXT( "NetworkFailures", "ServiceUnavailable", "Connection to \"PSN\" has been lost." );
#else
		const FText ReturnReason	= NSLOCTEXT( "NetworkFailures", "ServiceUnavailable", "Connection has been lost." );
#endif
		const FText OKButton		= NSLOCTEXT( "DialogButtons", "OKAY", "OK" );
		
		UWorld* const World = GetWorld();
		ASoldierGameMode* const GameMode = World != NULL ? Cast<ASoldierGameMode>(World->GetAuthGameMode()) : NULL;
		if (GameMode)
		{
			GameMode->AbortMatch();
		}
		
		ShowMessageThenGotoState( ReturnReason, OKButton, FText::GetEmpty(), SoldierGameInstanceState::MainMenu );
	}

	CurrentConnectionStatus = ConnectionStatus;
#endif
}

void USoldierGameInstance::HandleSessionFailure( const FUniqueNetId& NetId, ESessionFailure::Type FailureType )
{
	UE_LOG( LogOnlineGame, Warning, TEXT( "USoldierGameInstance::HandleSessionFailure: %u" ), (uint32)FailureType );

#if SHOOTER_CONSOLE_UI
	// If we are not currently at (or heading to) the welcome screen then display a message on consoles
	if (	OnlineMode != EOnlineMode::Offline &&
			PendingState != SoldierGameInstanceState::WelcomeScreen &&
			CurrentState != SoldierGameInstanceState::WelcomeScreen )
	{
		UE_LOG( LogOnlineGame, Log, TEXT( "USoldierGameInstance::HandleSessionFailure: Going to main menu" ) );

		// Display message on consoles
#if PLATFORM_XBOXONE
		const FText ReturnReason	= NSLOCTEXT( "NetworkFailures", "ServiceUnavailable", "Connection to Xbox LIVE has been lost." );
#elif PLATFORM_PS4
		const FText ReturnReason	= NSLOCTEXT( "NetworkFailures", "ServiceUnavailable", "Connection to PSN has been lost." );
#else
		const FText ReturnReason	= NSLOCTEXT( "NetworkFailures", "ServiceUnavailable", "Connection has been lost." );
#endif
		const FText OKButton		= NSLOCTEXT( "DialogButtons", "OKAY", "OK" );
		
		ShowMessageThenGotoState( ReturnReason, OKButton,  FText::GetEmpty(), SoldierGameInstanceState::MainMenu );
	}
#endif
}

void USoldierGameInstance::OnPreLoadMap(const FString& MapName)
{
	if (bPendingEnableSplitscreen)
	{
		// Allow splitscreen
		UGameViewportClient* GameViewportClient = GetGameViewportClient();
		if (GameViewportClient != nullptr)
		{
			GameViewportClient->SetForceDisableSplitscreen(false);

			bPendingEnableSplitscreen = false;
		}
	}
}

void USoldierGameInstance::OnPostLoadMap(UWorld*)
{
	// Make sure we hide the loading screen when the level is done loading
	USoldierGameViewportClient * SoldierViewport = Cast<USoldierGameViewportClient>(GetGameViewportClient());
	if (SoldierViewport != nullptr)
	{
		SoldierViewport->HideLoadingScreen();
	}
}

void USoldierGameInstance::OnUserCanPlayInvite(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults)
{
	CleanupOnlinePrivilegeTask();
	if (WelcomeMenuUI.IsValid())
	{
		WelcomeMenuUI->LockControls(false);
	}

	if (PrivilegeResults == (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures)	
	{
		if (UserId == *PendingInvite.UserId)
		{
			PendingInvite.bPrivilegesCheckedAndAllowed = true;
		}		
	}
	else
	{
		DisplayOnlinePrivilegeFailureDialogs(UserId, Privilege, PrivilegeResults);
		GotoState(SoldierGameInstanceState::WelcomeScreen);
	}
}

void USoldierGameInstance::OnUserCanPlayTogether(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults)
{
	CleanupOnlinePrivilegeTask();
	if (WelcomeMenuUI.IsValid())
	{
		WelcomeMenuUI->LockControls(false);
	}

	if (PrivilegeResults == (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures)
	{
		if (WelcomeMenuUI.IsValid())
		{
			WelcomeMenuUI->SetControllerAndAdvanceToMainMenu(PlayTogetherInfo.UserIndex);
		}
	}
	else
	{
		DisplayOnlinePrivilegeFailureDialogs(UserId, Privilege, PrivilegeResults);
		GotoState(SoldierGameInstanceState::WelcomeScreen);
	}
}

void USoldierGameInstance::OnPostDemoPlay()
{
	GotoState( SoldierGameInstanceState::Playing );
}

void USoldierGameInstance::HandleDemoPlaybackFailure( EDemoPlayFailure::Type FailureType, const FString& ErrorString )
{
	if (GetWorld() != nullptr && GetWorld()->WorldType == EWorldType::PIE)
	{
		UE_LOG(LogEngine, Warning, TEXT("Demo failed to play back correctly, got error %s"), *ErrorString);
		return;
	}

	ShowMessageThenGotoState(FText::Format(NSLOCTEXT("USoldierGameInstance", "DemoPlaybackFailedFmt", "Demo playback failed: {0}"), FText::FromString(ErrorString)), NSLOCTEXT("DialogButtons", "OKAY", "OK"), FText::GetEmpty(), SoldierGameInstanceState::MainMenu);
}

void USoldierGameInstance::StartGameInstance()
{
#if PLATFORM_PS4 == 0
	TCHAR Parm[4096] = TEXT("");

	const TCHAR* Cmd = FCommandLine::Get();

	// Catch the case where we want to override the map name on startup (used for connecting to other MP instances)
	if (FParse::Token(Cmd, Parm, UE_ARRAY_COUNT(Parm), 0) && Parm[0] != '-')
	{
		// if we're 'overriding' with the default map anyway, don't set a bogus 'playing' state.
		if (!MainMenuMap.Contains(Parm))
		{
			FURL DefaultURL;
			DefaultURL.LoadURLConfig(TEXT("DefaultPlayer"), GGameIni);

			FURL URL(&DefaultURL, Parm, TRAVEL_Partial);

			// If forcelan is set, we need to make sure to add the LAN flag to the travel url
			if (FParse::Param(Cmd, TEXT("forcelan")))
			{
				URL.AddOption(TEXT("bIsLanMatch"));
			}

			if (URL.Valid)
			{
				UEngine* const Engine = GetEngine();

				FString Error;

				const EBrowseReturnVal::Type BrowseRet = Engine->Browse(*WorldContext, URL, Error);

				if (BrowseRet == EBrowseReturnVal::Success)
				{
					// Success, we loaded the map, go directly to playing state
					GotoState(SoldierGameInstanceState::Playing);
					return;
				}
				else if (BrowseRet == EBrowseReturnVal::Pending)
				{
					// Assume network connection
					LoadFrontEndMap(MainMenuMap);
					AddNetworkFailureHandlers();
					ShowLoadingScreen();
					GotoState(SoldierGameInstanceState::Playing);
					return;
				}
			}
		}
	}
#endif

	GotoInitialState();
}

#if WITH_EDITOR

FGameInstancePIEResult USoldierGameInstance::StartPlayInEditorGameInstance(ULocalPlayer* LocalPlayer, const FGameInstancePIEParameters& Params)
{
	FWorldContext* PlayWorldContext = GetWorldContext();
	check(PlayWorldContext);

	UWorld* PlayWorld = PlayWorldContext->World();
	check(PlayWorld);

	FString CurrentMapName = PlayWorld->GetOutermost()->GetName();
	if (!PlayWorldContext->PIEPrefix.IsEmpty())
	{
		CurrentMapName.ReplaceInline(*PlayWorldContext->PIEPrefix, TEXT(""));
	}

#if SHOOTER_CONSOLE_UI
	if (CurrentMapName == WelcomeScreenMap)
	{
		GotoState(SoldierGameInstanceState::WelcomeScreen);
	}
	else 
#endif	// SHOOTER_CONSOLE_UI
	if (CurrentMapName == MainMenuMap)
	{
		GotoState(SoldierGameInstanceState::MainMenu);
	}
	else
	{
		GotoState(SoldierGameInstanceState::Playing);
	}

	return Super::StartPlayInEditorGameInstance(LocalPlayer, Params);
}

#endif	// WITH_EDITOR

FName USoldierGameInstance::GetInitialState()
{
#if SHOOTER_CONSOLE_UI	
	// Start in the welcome screen state on consoles
	return SoldierGameInstanceState::WelcomeScreen;
#else
	// On PC, go directly to the main menu
	return SoldierGameInstanceState::MainMenu;
#endif
}

void USoldierGameInstance::GotoInitialState()
{
	GotoState(GetInitialState());
}

const FName USoldierGameInstance::GetCurrentState() const 
{
	return CurrentState;
}

void USoldierGameInstance::ShowMessageThenGotoState( const FText& Message, const FText& OKButtonString, const FText& CancelButtonString, const FName& NewState, const bool OverrideExisting, TWeakObjectPtr< ULocalPlayer > PlayerOwner )
{
	UE_LOG( LogOnline, Log, TEXT( "ShowMessageThenGotoState: Message: %s, NewState: %s" ), *Message.ToString(), *NewState.ToString() );

	const bool bAtWelcomeScreen = PendingState == SoldierGameInstanceState::WelcomeScreen || CurrentState == SoldierGameInstanceState::WelcomeScreen;

	// Never override the welcome screen
	if ( bAtWelcomeScreen )
	{
		UE_LOG( LogOnline, Log, TEXT( "ShowMessageThenGotoState: Ignoring due to higher message priority in queue (at welcome screen)." ) );
		return;
	}

	const bool bAlreadyAtMessageMenu = PendingState == SoldierGameInstanceState::MessageMenu || CurrentState == SoldierGameInstanceState::MessageMenu;
	const bool bAlreadyAtDestState = PendingState == NewState || CurrentState == NewState;

	// If we are already going to the message menu, don't override unless asked to
	if ( bAlreadyAtMessageMenu && PendingMessage.NextState == NewState && !OverrideExisting )
	{
		UE_LOG( LogOnline, Log, TEXT( "ShowMessageThenGotoState: Ignoring due to higher message priority in queue (check 1)." ) );
		return;
	}

	// If we are already going to the message menu, and the next dest is welcome screen, don't override
	if ( bAlreadyAtMessageMenu && PendingMessage.NextState == SoldierGameInstanceState::WelcomeScreen )
	{
		UE_LOG( LogOnline, Log, TEXT( "ShowMessageThenGotoState: Ignoring due to higher message priority in queue (check 2)." ) );
		return;
	}

	// If we are already at the dest state, don't override unless asked
	if ( bAlreadyAtDestState && !OverrideExisting )
	{
		UE_LOG( LogOnline, Log, TEXT( "ShowMessageThenGotoState: Ignoring due to higher message priority in queue (check 3)" ) );
		return;
	}

	PendingMessage.DisplayString		= Message;
	PendingMessage.OKButtonString		= OKButtonString;
	PendingMessage.CancelButtonString	= CancelButtonString;
	PendingMessage.NextState			= NewState;
	PendingMessage.PlayerOwner			= PlayerOwner;

	if ( CurrentState == SoldierGameInstanceState::MessageMenu )
	{
		UE_LOG( LogOnline, Log, TEXT( "ShowMessageThenGotoState: Forcing new message" ) );
		EndMessageMenuState();
		BeginMessageMenuState();
	}
	else
	{
		GotoState(SoldierGameInstanceState::MessageMenu);
	}
}

void USoldierGameInstance::ShowLoadingScreen()
{
	// This can be confusing, so here is what is happening:
	//	For LoadMap, we use the ISoldierGameLoadingScreenModule interface to show the load screen
	//  This is necessary since this is a blocking call, and our viewport loading screen won't get updated.
	//  We can't use ISoldierGameLoadingScreenModule for seamless travel though
	//  In this case, we just add a widget to the viewport, and have it update on the main thread
	//  To simplify things, we just do both, and you can't tell, one will cover the other if they both show at the same time
	ISoldierGameLoadingScreenModule* const LoadingScreenModule = FModuleManager::LoadModulePtr<ISoldierGameLoadingScreenModule>("SoldierGameLoadingScreen");
	if (LoadingScreenModule != nullptr)
	{
		LoadingScreenModule->StartInGameLoadingScreen();
	}

	USoldierGameViewportClient * SoldierViewport = Cast<USoldierGameViewportClient>(GetGameViewportClient());

	if ( SoldierViewport != NULL )
	{
		SoldierViewport->ShowLoadingScreen();
	}
}

bool USoldierGameInstance::LoadFrontEndMap(const FString& MapName)
{
	bool bSuccess = true;

	// if already loaded, do nothing
	UWorld* const World = GetWorld();
	if (World)
	{
		FString const CurrentMapName = *World->PersistentLevel->GetOutermost()->GetName();
		//if (MapName.Find(TEXT("Highrise")) != -1)
		if (CurrentMapName == MapName)
		{
			return bSuccess;
		}
	}

	FString Error;
	EBrowseReturnVal::Type BrowseRet = EBrowseReturnVal::Failure;
	FURL URL(
		*FString::Printf(TEXT("%s"), *MapName)
		);

	if (URL.Valid && !HasAnyFlags(RF_ClassDefaultObject)) //CastChecked<UEngine>() will fail if using Default__SoldierGameInstance, so make sure that we're not default
	{
		BrowseRet = GetEngine()->Browse(*WorldContext, URL, Error);

		// Handle failure.
		if (BrowseRet != EBrowseReturnVal::Success)
		{
			UE_LOG(LogLoad, Fatal, TEXT("%s"), *FString::Printf(TEXT("Failed to enter %s: %s. Please check the log for errors."), *MapName, *Error));
			bSuccess = false;
		}
	}
	return bSuccess;
}

ASoldierGameSession* USoldierGameInstance::GetGameSession() const
{
	UWorld* const World = GetWorld();
	if (World)
	{
		AGameModeBase* const Game = World->GetAuthGameMode();
		if (Game)
		{
			return Cast<ASoldierGameSession>(Game->GameSession);
		}
	}

	return nullptr;
}

void USoldierGameInstance::TravelLocalSessionFailure(UWorld *World, ETravelFailure::Type FailureType, const FString& ReasonString)
{
	ASoldierPlayerController_Menu* const FirstPC = Cast<ASoldierPlayerController_Menu>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (FirstPC != nullptr)
	{
		FText ReturnReason = NSLOCTEXT("NetworkErrors", "JoinSessionFailed", "Join Session failed.");
		if (ReasonString.IsEmpty() == false)
		{
			ReturnReason = FText::Format(NSLOCTEXT("NetworkErrors", "JoinSessionFailedReasonFmt", "Join Session failed. {0}"), FText::FromString(ReasonString));
		}

		FText OKButton = NSLOCTEXT("DialogButtons", "OKAY", "OK");
		ShowMessageThenGoMain(ReturnReason, OKButton, FText::GetEmpty());
	}
}

void USoldierGameInstance::ShowMessageThenGoMain(const FText& Message, const FText& OKButtonString, const FText& CancelButtonString)
{
	ShowMessageThenGotoState(Message, OKButtonString, CancelButtonString, SoldierGameInstanceState::MainMenu);
}

void USoldierGameInstance::SetPendingInvite(const FSoldierPendingInvite& InPendingInvite)
{
	PendingInvite = InPendingInvite;
}

void USoldierGameInstance::GotoState(FName NewState)
{
	UE_LOG( LogOnline, Log, TEXT( "GotoState: NewState: %s" ), *NewState.ToString() );

	PendingState = NewState;
}

void USoldierGameInstance::MaybeChangeState()
{
	if ( (PendingState != CurrentState) && (PendingState != SoldierGameInstanceState::None) )
	{
		FName const OldState = CurrentState;

		// end current state
		EndCurrentState(PendingState);

		// begin new state
		BeginNewState(PendingState, OldState);

		// clear pending change
		PendingState = SoldierGameInstanceState::None;
	}
}

void USoldierGameInstance::EndCurrentState(FName NextState)
{
	// per-state custom ending code here
	if (CurrentState == SoldierGameInstanceState::PendingInvite)
	{
		EndPendingInviteState();
	}
	else if (CurrentState == SoldierGameInstanceState::WelcomeScreen)
	{
		EndWelcomeScreenState();
	}
	else if (CurrentState == SoldierGameInstanceState::MainMenu)
	{
		EndMainMenuState();
	}
	else if (CurrentState == SoldierGameInstanceState::MessageMenu)
	{
		EndMessageMenuState();
	}
	else if (CurrentState == SoldierGameInstanceState::Playing)
	{
		EndPlayingState();
	}

	CurrentState = SoldierGameInstanceState::None;
}

void USoldierGameInstance::BeginNewState(FName NewState, FName PrevState)
{
	// per-state custom starting code here

	if (NewState == SoldierGameInstanceState::PendingInvite)
	{
		BeginPendingInviteState();
	}
	else if (NewState == SoldierGameInstanceState::WelcomeScreen)
	{
		BeginWelcomeScreenState();
	}
	else if (NewState == SoldierGameInstanceState::MainMenu)
	{
		BeginMainMenuState();
	}
	else if (NewState == SoldierGameInstanceState::MessageMenu)
	{
		BeginMessageMenuState();
	}
	else if (NewState == SoldierGameInstanceState::Playing)
	{
		BeginPlayingState();
	}

	CurrentState = NewState;
}

void USoldierGameInstance::BeginPendingInviteState()
{	
	if (LoadFrontEndMap(MainMenuMap))
	{				
		StartOnlinePrivilegeTask(IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateUObject(this, &USoldierGameInstance::OnUserCanPlayInvite), EUserPrivileges::CanPlayOnline, PendingInvite.UserId);
	}
	else
	{
		GotoState(SoldierGameInstanceState::WelcomeScreen);
	}
}

void USoldierGameInstance::EndPendingInviteState()
{
	// cleanup in case the state changed before the pending invite was handled.
	CleanupOnlinePrivilegeTask();
}

void USoldierGameInstance::BeginWelcomeScreenState()
{
	//this must come before split screen player removal so that the OSS sets all players to not using online features.
	SetOnlineMode(EOnlineMode::Offline);

	// Remove any possible splitscren players
	RemoveSplitScreenPlayers();

	LoadFrontEndMap(WelcomeScreenMap);

	ULocalPlayer* const LocalPlayer = GetFirstGamePlayer();
	LocalPlayer->SetCachedUniqueNetId(nullptr);
	check(!WelcomeMenuUI.IsValid());
	WelcomeMenuUI = MakeShareable(new FSoldierWelcomeMenu);
	WelcomeMenuUI->Construct( this );
	WelcomeMenuUI->AddToGameViewport();

	// Disallow splitscreen (we will allow while in the playing state)
	GetGameViewportClient()->SetForceDisableSplitscreen( true );
}

void USoldierGameInstance::EndWelcomeScreenState()
{
	if (WelcomeMenuUI.IsValid())
	{
		WelcomeMenuUI->RemoveFromGameViewport();
		WelcomeMenuUI = nullptr;
	}
}

void USoldierGameInstance::SetPresenceForLocalPlayers(const FString& StatusStr, const FVariantData& PresenceData)
{
	const IOnlinePresencePtr Presence = Online::GetPresenceInterface(GetWorld());
	if (Presence.IsValid())
	{
		for (int i = 0; i < LocalPlayers.Num(); ++i)
		{
			FUniqueNetIdRepl UserId = LocalPlayers[i]->GetPreferredUniqueNetId();

			if (UserId.IsValid())
			{
				FOnlineUserPresenceStatus PresenceStatus;
				PresenceStatus.StatusStr = StatusStr;
				PresenceStatus.State = EOnlinePresenceState::Online;
				PresenceStatus.Properties.Add(DefaultPresenceKey, PresenceData);

				Presence->SetPresence(*UserId, PresenceStatus);
			}
		}
	}
}

void USoldierGameInstance::BeginMainMenuState()
{
	// Make sure we're not showing the loadscreen
	USoldierGameViewportClient * SoldierViewport = Cast<USoldierGameViewportClient>(GetGameViewportClient());

	if ( SoldierViewport != NULL )
	{
		SoldierViewport->HideLoadingScreen();
	}

	SetOnlineMode(EOnlineMode::Offline);

	// Disallow splitscreen
	UGameViewportClient* GameViewportClient = GetGameViewportClient();
	
	if (GameViewportClient)
	{
		GetGameViewportClient()->SetForceDisableSplitscreen(true);
	}

	// Remove any possible splitscren players
	RemoveSplitScreenPlayers();

	// Set presence to menu state for the owning player
	SetPresenceForLocalPlayers(FString(TEXT("In Menu")), FVariantData(FString(TEXT("OnMenu"))));

	// load startup map
	LoadFrontEndMap(MainMenuMap);

	// player 0 gets to own the UI
	ULocalPlayer* const Player = GetFirstGamePlayer();

	MainMenuUI = MakeShareable(new FSoldierMainMenu());
	MainMenuUI->Construct(this, Player);
	MainMenuUI->AddMenuToGameViewport();

	// It's possible that a play together event was sent by the system while the player was in-game or didn't
	// have the application launched. The game will automatically go directly to the main menu state in those cases
	// so this will handle Play Together if that is why we transitioned here.
	if (PlayTogetherInfo.UserIndex != -1)
	{
		MainMenuUI->OnPlayTogetherEventReceived();
	}

#if !SHOOTER_CONSOLE_UI
	// The cached unique net ID is usually set on the welcome screen, but there isn't
	// one on PC/Mac, so do it here.
	if (Player != nullptr)
	{
		Player->SetControllerId(0);
		Player->SetCachedUniqueNetId(Player->GetUniqueNetIdFromCachedControllerId().GetUniqueNetId());
	}
#endif

	RemoveNetworkFailureHandlers();
}

void USoldierGameInstance::EndMainMenuState()
{
	if (MainMenuUI.IsValid())
	{
		MainMenuUI->RemoveMenuFromGameViewport();
		MainMenuUI = nullptr;
	}
}

void USoldierGameInstance::BeginMessageMenuState()
{
	if (PendingMessage.DisplayString.IsEmpty())
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("USoldierGameInstance::BeginMessageMenuState: Display string is empty"));
		GotoInitialState();
		return;
	}

	// Make sure we're not showing the loadscreen
	USoldierGameViewportClient * SoldierViewport = Cast<USoldierGameViewportClient>(GetGameViewportClient());

	if ( SoldierViewport != NULL )
	{
		SoldierViewport->HideLoadingScreen();
	}

	check(!MessageMenuUI.IsValid());
	MessageMenuUI = MakeShareable(new FSoldierMessageMenu);
	MessageMenuUI->Construct(this, PendingMessage.PlayerOwner, PendingMessage.DisplayString, PendingMessage.OKButtonString, PendingMessage.CancelButtonString, PendingMessage.NextState);

	PendingMessage.DisplayString = FText::GetEmpty();
}

void USoldierGameInstance::EndMessageMenuState()
{
	if (MessageMenuUI.IsValid())
	{
		MessageMenuUI->RemoveFromGameViewport();
		MessageMenuUI = nullptr;
	}
}

void USoldierGameInstance::BeginPlayingState()
{
	bPendingEnableSplitscreen = true;

	// Set presence for playing in a map
	SetPresenceForLocalPlayers(FString(TEXT("In Game")), FVariantData(FString(TEXT("InGame"))));

	// Make sure viewport has focus
	FSlateApplication::Get().SetAllUserFocusToGameViewport();
}

void USoldierGameInstance::EndPlayingState()
{
	// Disallow splitscreen
	GetGameViewportClient()->SetForceDisableSplitscreen( true );

	// Clear the players' presence information
	SetPresenceForLocalPlayers(FString(TEXT("In Menu")), FVariantData(FString(TEXT("OnMenu"))));

	UWorld* const World = GetWorld();
	ASoldierGameState* const GameState = World != NULL ? World->GetGameState<ASoldierGameState>() : NULL;

	if (GameState)
	{
		// Send round end events for local players
		for (int i = 0; i < LocalPlayers.Num(); ++i)
		{
			ASoldierPlayerController* SoldierPC = Cast<ASoldierPlayerController>(LocalPlayers[i]->PlayerController);
			if (SoldierPC)
			{
				// Assuming you can't win if you quit early
				SoldierPC->ClientSendRoundEndEvent(false, GameState->ElapsedTime);
			}
		}

		// Give the game state a chance to cleanup first
		GameState->RequestFinishAndExitToMainMenu();
	}
	else
	{
		// If there is no game state, make sure the session is in a good state
		CleanupSessionOnReturnToMenu();
	}
}

void USoldierGameInstance::OnEndSessionComplete( FName SessionName, bool bWasSuccessful )
{
	UE_LOG(LogOnline, Log, TEXT("USoldierGameInstance::OnEndSessionComplete: Session=%s bWasSuccessful=%s"), *SessionName.ToString(), bWasSuccessful ? TEXT("true") : TEXT("false") );

	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnStartSessionCompleteDelegate_Handle  (OnStartSessionCompleteDelegateHandle);
			Sessions->ClearOnEndSessionCompleteDelegate_Handle    (OnEndSessionCompleteDelegateHandle);
			Sessions->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
		}
	}

	// continue
	CleanupSessionOnReturnToMenu();
}

void USoldierGameInstance::CleanupSessionOnReturnToMenu()
{
	bool bPendingOnlineOp = false;

	// end online game and then destroy it
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr Sessions = ( OnlineSub != NULL ) ? OnlineSub->GetSessionInterface() : NULL;

	if ( Sessions.IsValid() )
	{
		FName GameSession(NAME_GameSession);
		EOnlineSessionState::Type SessionState = Sessions->GetSessionState(NAME_GameSession);
		UE_LOG(LogOnline, Log, TEXT("Session %s is '%s'"), *GameSession.ToString(), EOnlineSessionState::ToString(SessionState));

		if ( EOnlineSessionState::InProgress == SessionState )
		{
			UE_LOG(LogOnline, Log, TEXT("Ending session %s on return to main menu"), *GameSession.ToString() );
			OnEndSessionCompleteDelegateHandle = Sessions->AddOnEndSessionCompleteDelegate_Handle(OnEndSessionCompleteDelegate);
			Sessions->EndSession(NAME_GameSession);
			bPendingOnlineOp = true;
		}
		else if ( EOnlineSessionState::Ending == SessionState )
		{
			UE_LOG(LogOnline, Log, TEXT("Waiting for session %s to end on return to main menu"), *GameSession.ToString() );
			OnEndSessionCompleteDelegateHandle = Sessions->AddOnEndSessionCompleteDelegate_Handle(OnEndSessionCompleteDelegate);
			bPendingOnlineOp = true;
		}
		else if ( EOnlineSessionState::Ended == SessionState || EOnlineSessionState::Pending == SessionState )
		{
			UE_LOG(LogOnline, Log, TEXT("Destroying session %s on return to main menu"), *GameSession.ToString() );
			OnDestroySessionCompleteDelegateHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(OnEndSessionCompleteDelegate);
			Sessions->DestroySession(NAME_GameSession);
			bPendingOnlineOp = true;
		}
		else if ( EOnlineSessionState::Starting == SessionState || EOnlineSessionState::Creating == SessionState)
		{
			UE_LOG(LogOnline, Log, TEXT("Waiting for session %s to start, and then we will end it to return to main menu"), *GameSession.ToString() );
			OnStartSessionCompleteDelegateHandle = Sessions->AddOnStartSessionCompleteDelegate_Handle(OnEndSessionCompleteDelegate);
			bPendingOnlineOp = true;
		}
	}

	if ( !bPendingOnlineOp )
	{
		//GEngine->HandleDisconnect( GetWorld(), GetWorld()->GetNetDriver() );
	}
}

void USoldierGameInstance::LabelPlayerAsQuitter(ULocalPlayer* LocalPlayer) const
{
	ASoldierPlayerState* const PlayerState = LocalPlayer && LocalPlayer->PlayerController ? Cast<ASoldierPlayerState>(LocalPlayer->PlayerController->PlayerState) : nullptr;	
	if(PlayerState)
	{
		PlayerState->SetQuitter(true);
	}
}

void USoldierGameInstance::RemoveNetworkFailureHandlers()
{
	// Remove the local session/travel failure bindings if they exist
	if (GEngine->OnTravelFailure().IsBoundToObject(this) == true)
	{
		GEngine->OnTravelFailure().Remove(TravelLocalSessionFailureDelegateHandle);
	}
}

void USoldierGameInstance::AddNetworkFailureHandlers()
{
	// Add network/travel error handlers (if they are not already there)
	if (GEngine->OnTravelFailure().IsBoundToObject(this) == false)
	{
		TravelLocalSessionFailureDelegateHandle = GEngine->OnTravelFailure().AddUObject(this, &USoldierGameInstance::TravelLocalSessionFailure);
	}
}

TSubclassOf<UOnlineSession> USoldierGameInstance::GetOnlineSessionClass()
{
	return USoldierOnlineSessionClient::StaticClass();
}

bool USoldierGameInstance::HostQuickSession(ULocalPlayer& LocalPlayer, const FOnlineSessionSettings& SessionSettings)
{
	// This function is different from BeginHostingQuickMatch in that it creates a session and then starts a quick match,
	// while BeginHostingQuickMatch assumes a session already exists

	if (ASoldierGameSession* const GameSession = GetGameSession())
	{
		// Add callback delegate for completion
		OnCreatePresenceSessionCompleteDelegateHandle = GameSession->OnCreatePresenceSessionComplete().AddUObject(this, &USoldierGameInstance::OnCreatePresenceSessionComplete);

		TravelURL = GetQuickMatchUrl();

		FOnlineSessionSettings HostSettings = SessionSettings;

		const FString GameType = UGameplayStatics::ParseOption(TravelURL, TEXT("game"));

		// Determine the map name from the travelURL
		const FString MapNameSubStr = "/Game/Maps/";
		const FString ChoppedMapName = TravelURL.RightChop(MapNameSubStr.Len());
		const FString MapName = ChoppedMapName.LeftChop(ChoppedMapName.Len() - ChoppedMapName.Find("?game"));

		HostSettings.Set(SETTING_GAMEMODE, GameType, EOnlineDataAdvertisementType::ViaOnlineService);
		HostSettings.Set(SETTING_MAPNAME, MapName, EOnlineDataAdvertisementType::ViaOnlineService);
		HostSettings.NumPublicConnections = 16;

		if (GameSession->HostSession(LocalPlayer.GetPreferredUniqueNetId().GetUniqueNetId(), NAME_GameSession, SessionSettings))
		{
			// If any error occurred in the above, pending state would be set
			if (PendingState == CurrentState || PendingState == SoldierGameInstanceState::None)
			{
				// Go ahead and go into loading state now
				// If we fail, the delegate will handle showing the proper messaging and move to the correct state
				ShowLoadingScreen();
				GotoState(SoldierGameInstanceState::Playing);
				return true;
			}
		}
	}

	return false;
}

bool USoldierGameInstance::HostGame(ULocalPlayer* LocalPlayer, const FString& GameType, const FString& InTravelURL)
{
	if (GetOnlineMode() == EOnlineMode::Offline)
	{
		//
		// Offline game, just go straight to map
		//

		ShowLoadingScreen();
		GotoState(SoldierGameInstanceState::Playing);

		// Travel to the specified match URL
		TravelURL = InTravelURL;
		GetWorld()->ServerTravel(TravelURL);
		return true;
	}

	//
	// Online game
	//

	ASoldierGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		// add callback delegate for completion
		OnCreatePresenceSessionCompleteDelegateHandle = GameSession->OnCreatePresenceSessionComplete().AddUObject(this, &USoldierGameInstance::OnCreatePresenceSessionComplete);

		TravelURL = InTravelURL;
		bool const bIsLanMatch = InTravelURL.Contains(TEXT("?bIsLanMatch"));

		//determine the map name from the travelURL
		const FString& MapNameSubStr = "/Game/Maps/";
		const FString& ChoppedMapName = TravelURL.RightChop(MapNameSubStr.Len());
		const FString& MapName = ChoppedMapName.LeftChop(ChoppedMapName.Len() - ChoppedMapName.Find("?game"));

		if (GameSession->HostSession(LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId(), NAME_GameSession, GameType, MapName, bIsLanMatch, true, ASoldierGameSession::DEFAULT_NUM_PLAYERS))
		{
			// If any error occurred in the above, pending state would be set
			if ( (PendingState == CurrentState) || (PendingState == SoldierGameInstanceState::None) )
			{
				// Go ahead and go into loading state now
				// If we fail, the delegate will handle showing the proper messaging and move to the correct state
				ShowLoadingScreen();
				GotoState(SoldierGameInstanceState::Playing);
				return true;
			}
		}
	}

	return false;
}

bool USoldierGameInstance::JoinSession(ULocalPlayer* LocalPlayer, int32 SessionIndexInSearchResults)
{
	// needs to tear anything down based on current state?

	ASoldierGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		AddNetworkFailureHandlers();

		OnJoinSessionCompleteDelegateHandle = GameSession->OnJoinSessionComplete().AddUObject(this, &USoldierGameInstance::OnJoinSessionComplete);
		if (GameSession->JoinSession(LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId(), NAME_GameSession, SessionIndexInSearchResults))
		{
			// If any error occured in the above, pending state would be set
			if ( (PendingState == CurrentState) || (PendingState == SoldierGameInstanceState::None) )
			{
				// Go ahead and go into loading state now
				// If we fail, the delegate will handle showing the proper messaging and move to the correct state
				ShowLoadingScreen();
				GotoState(SoldierGameInstanceState::Playing);
				return true;
			}
		}
	}

	return false;
}

bool USoldierGameInstance::JoinSession(ULocalPlayer* LocalPlayer, const FOnlineSessionSearchResult& SearchResult)
{
	// needs to tear anything down based on current state?
	ASoldierGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		AddNetworkFailureHandlers();

		OnJoinSessionCompleteDelegateHandle = GameSession->OnJoinSessionComplete().AddUObject(this, &USoldierGameInstance::OnJoinSessionComplete);
		if (GameSession->JoinSession(LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId(), NAME_GameSession, SearchResult))
		{
			// If any error occured in the above, pending state would be set
			if ( (PendingState == CurrentState) || (PendingState == SoldierGameInstanceState::None) )
			{
				// Go ahead and go into loading state now
				// If we fail, the delegate will handle showing the proper messaging and move to the correct state
				ShowLoadingScreen();
				GotoState(SoldierGameInstanceState::Playing);
				return true;
			}
		}
	}

	return false;
}

bool USoldierGameInstance::PlayDemo(ULocalPlayer* LocalPlayer, const FString& DemoName)
{
	ShowLoadingScreen();

	// Play the demo
	PlayReplay(DemoName);
	
	return true;
}

/** Callback which is intended to be called upon finding sessions */
void USoldierGameInstance::OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result)
{
	// unhook the delegate
	ASoldierGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		GameSession->OnJoinSessionComplete().Remove(OnJoinSessionCompleteDelegateHandle);
	}

	// Add the splitscreen player if one exists
	if (Result == EOnJoinSessionCompleteResult::Success && LocalPlayers.Num() > 1)
	{
		IOnlineSessionPtr Sessions = Online::GetSessionInterface(GetWorld());
		if (Sessions.IsValid() && LocalPlayers[1]->GetPreferredUniqueNetId().IsValid())
		{
			Sessions->RegisterLocalPlayer(*LocalPlayers[1]->GetPreferredUniqueNetId(), NAME_GameSession,
				FOnRegisterLocalPlayerCompleteDelegate::CreateUObject(this, &USoldierGameInstance::OnRegisterJoiningLocalPlayerComplete));
		}
	}
	else
	{
		// We either failed or there is only a single local user
		FinishJoinSession(Result);
	}
}

void USoldierGameInstance::FinishJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		FText ReturnReason;
		switch (Result)
		{
		case EOnJoinSessionCompleteResult::SessionIsFull:
			ReturnReason = NSLOCTEXT("NetworkErrors", "JoinSessionFailed", "Game is full.");
			break;
		case EOnJoinSessionCompleteResult::SessionDoesNotExist:
			ReturnReason = NSLOCTEXT("NetworkErrors", "JoinSessionFailed", "Game no longer exists.");
			break;
		default:
			ReturnReason = NSLOCTEXT("NetworkErrors", "JoinSessionFailed", "Join failed.");
			break;
		}

		FText OKButton = NSLOCTEXT("DialogButtons", "OKAY", "OK");
		RemoveNetworkFailureHandlers();
		ShowMessageThenGoMain(ReturnReason, OKButton, FText::GetEmpty());
		return;
	}

	InternalTravelToSession(NAME_GameSession);
}

void USoldierGameInstance::OnRegisterJoiningLocalPlayerComplete(const FUniqueNetId& PlayerId, EOnJoinSessionCompleteResult::Type Result)
{
	FinishJoinSession(Result);
}

void USoldierGameInstance::InternalTravelToSession(const FName& SessionName)
{
	APlayerController * const PlayerController = GetFirstLocalPlayerController();

	if ( PlayerController == nullptr )
	{
		FText ReturnReason = NSLOCTEXT("NetworkErrors", "InvalidPlayerController", "Invalid Player Controller");
		FText OKButton = NSLOCTEXT("DialogButtons", "OKAY", "OK");
		RemoveNetworkFailureHandlers();
		ShowMessageThenGoMain(ReturnReason, OKButton, FText::GetEmpty());
		return;
	}

	// travel to session
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());

	if ( OnlineSub == nullptr )
	{
		FText ReturnReason = NSLOCTEXT("NetworkErrors", "OSSMissing", "OSS missing");
		FText OKButton = NSLOCTEXT("DialogButtons", "OKAY", "OK");
		RemoveNetworkFailureHandlers();
		ShowMessageThenGoMain(ReturnReason, OKButton, FText::GetEmpty());
		return;
	}

	FString URL;
	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

	if ( !Sessions.IsValid() || !Sessions->GetResolvedConnectString( SessionName, URL ) )
	{
		FText FailReason = NSLOCTEXT("NetworkErrors", "TravelSessionFailed", "Travel to Session failed.");
		FText OKButton = NSLOCTEXT("DialogButtons", "OKAY", "OK");
		ShowMessageThenGoMain(FailReason, OKButton, FText::GetEmpty());
		UE_LOG(LogOnlineGame, Warning, TEXT("Failed to travel to session upon joining it"));
		return;
	}

	// Add debug encryption token if desired.
	if (CVarSoldierGameTestEncryption->GetInt() != 0)
	{
		// This is just a value for testing/debugging, the server will use the same key regardless of the token value.
		// But the token could be a user ID and/or session ID that would be used to generate a unique key per user and/or session, if desired.
		URL += TEXT("?EncryptionToken=1");
	}

	PlayerController->ClientTravel(URL, TRAVEL_Absolute);
}

/** Callback which is intended to be called upon session creation */
void USoldierGameInstance::OnCreatePresenceSessionComplete(FName SessionName, bool bWasSuccessful)
{
	ASoldierGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		GameSession->OnCreatePresenceSessionComplete().Remove(OnCreatePresenceSessionCompleteDelegateHandle);

		// Add the splitscreen player if one exists
		if (bWasSuccessful && LocalPlayers.Num() > 1)
		{
			IOnlineSessionPtr Sessions = Online::GetSessionInterface(GetWorld());
			if (Sessions.IsValid() && LocalPlayers[1]->GetPreferredUniqueNetId().IsValid())
			{
				Sessions->RegisterLocalPlayer(*LocalPlayers[1]->GetPreferredUniqueNetId(), NAME_GameSession,
					FOnRegisterLocalPlayerCompleteDelegate::CreateUObject(this, &USoldierGameInstance::OnRegisterLocalPlayerComplete));
			}
		}
		else
		{
			// We either failed or there is only a single local user
			FinishSessionCreation(bWasSuccessful ? EOnJoinSessionCompleteResult::Success : EOnJoinSessionCompleteResult::UnknownError);
		}
	}
}

/** Initiates the session searching */
bool USoldierGameInstance::FindSessions(ULocalPlayer* PlayerOwner, bool bIsDedicatedServer, bool bFindLAN)
{
	bool bResult = false;

	check(PlayerOwner != nullptr);
	if (PlayerOwner)
	{
		ASoldierGameSession* const GameSession = GetGameSession();
		if (GameSession)
		{
			GameSession->OnFindSessionsComplete().RemoveAll(this);
			OnSearchSessionsCompleteDelegateHandle = GameSession->OnFindSessionsComplete().AddUObject(this, &USoldierGameInstance::OnSearchSessionsComplete);

			GameSession->FindSessions(PlayerOwner->GetPreferredUniqueNetId().GetUniqueNetId(), NAME_GameSession, bFindLAN, !bIsDedicatedServer);

			bResult = true;
		}
	}

	return bResult;
}

/** Callback which is intended to be called upon finding sessions */
void USoldierGameInstance::OnSearchSessionsComplete(bool bWasSuccessful)
{
	ASoldierGameSession* const Session = GetGameSession();
	if (Session)
	{
		Session->OnFindSessionsComplete().Remove(OnSearchSessionsCompleteDelegateHandle);
	}
}

bool USoldierGameInstance::Tick(float DeltaSeconds)
{
	// Dedicated server doesn't need to worry about game state
	if (IsDedicatedServerInstance() == true)
	{
		return true;
	}

	USoldierGameViewportClient* SoldierViewport = Cast<USoldierGameViewportClient>(GetGameViewportClient());
	if (FSlateApplication::IsInitialized() && SoldierViewport != nullptr)
	{
		if (FSlateApplication::Get().GetGameViewport() != SoldierViewport->GetGameViewportWidget())
		{
			return true;
		}
	}

	// Because this takes place outside the normal UWorld tick, we need to register what world we're ticking/modifying here to avoid issues in the editor
	FScopedConditionalWorldSwitcher WorldSwitcher(GetWorld());

	MaybeChangeState();

	if (CurrentState != SoldierGameInstanceState::WelcomeScreen && SoldierViewport != nullptr)
	{
		// If at any point we aren't licensed (but we are after welcome screen) bounce them back to the welcome screen
		if (!bIsLicensed && CurrentState != SoldierGameInstanceState::None && SoldierViewport != nullptr && !SoldierViewport->IsShowingDialog())
		{
			const FText ReturnReason	= NSLOCTEXT( "ProfileMessages", "NeedLicense", "The signed in users do not have a license for this game. Please purchase SoldierGame from the Xbox Marketplace or sign in a user with a valid license." );
			const FText OKButton		= NSLOCTEXT( "DialogButtons", "OKAY", "OK" );

			ShowMessageThenGotoState( ReturnReason, OKButton, FText::GetEmpty(), SoldierGameInstanceState::WelcomeScreen );
		}

		// Show controller disconnected dialog if any local players have an invalid controller
		if (!SoldierViewport->IsShowingDialog())
		{
			for (int i = 0; i < LocalPlayers.Num(); ++i)
			{
				if (LocalPlayers[i] && LocalPlayers[i]->GetControllerId() == -1)
				{
					SoldierViewport->ShowDialog( 
						LocalPlayers[i],
						ESoldierDialogType::ControllerDisconnected,
						FText::Format(NSLOCTEXT("ProfileMessages", "PlayerReconnectControllerFmt", "Player {0}, please reconnect your controller."), FText::AsNumber(i + 1)),
#if PLATFORM_PS4
						NSLOCTEXT("DialogButtons", "PS4_CrossButtonContinue", "Cross Button - Continue"),
#else
						NSLOCTEXT("DialogButtons", "AButtonContinue", "A - Continue"),
#endif
						FText::GetEmpty(),
						FOnClicked::CreateUObject(this, &USoldierGameInstance::OnControllerReconnectConfirm),
						FOnClicked()
					);
				}
			}
		}
	}

	// If we have a pending invite, and we are at the welcome screen, and the session is properly shut down, accept it
	if (PendingInvite.UserId.IsValid() && PendingInvite.bPrivilegesCheckedAndAllowed && CurrentState == SoldierGameInstanceState::PendingInvite)
	{
		IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
		IOnlineSessionPtr Sessions = (OnlineSub != NULL) ? OnlineSub->GetSessionInterface() : NULL;

		if (Sessions.IsValid())
		{
			EOnlineSessionState::Type SessionState = Sessions->GetSessionState(NAME_GameSession);

			if (SessionState == EOnlineSessionState::NoSession)
			{
				ULocalPlayer * NewPlayerOwner = GetFirstGamePlayer();

				if (NewPlayerOwner != nullptr)
				{
					NewPlayerOwner->SetControllerId(PendingInvite.ControllerId);
					NewPlayerOwner->SetCachedUniqueNetId(PendingInvite.UserId);
					SetOnlineMode(EOnlineMode::Online);

					const bool bIsLocalPlayerHost = PendingInvite.UserId.IsValid() && PendingInvite.InviteResult.Session.OwningUserId.IsValid() && *PendingInvite.UserId == *PendingInvite.InviteResult.Session.OwningUserId;
					if (bIsLocalPlayerHost)
					{
						HostQuickSession(*NewPlayerOwner, PendingInvite.InviteResult.Session.SessionSettings);
					}
					else
					{
						JoinSession(NewPlayerOwner, PendingInvite.InviteResult);
					}
				}

				PendingInvite.UserId.Reset();
			}
		}
	}

	return true;
}

bool USoldierGameInstance::HandleOpenCommand(const TCHAR* Cmd, FOutputDevice& Ar, UWorld* InWorld)
{
	bool const bOpenSuccessful = Super::HandleOpenCommand(Cmd, Ar, InWorld);
	if (bOpenSuccessful)
	{
		GotoState(SoldierGameInstanceState::Playing);
	}

	return bOpenSuccessful;
}

bool USoldierGameInstance::HandleDisconnectCommand(const TCHAR* Cmd, FOutputDevice& Ar, UWorld* InWorld)
{
	bool const bDisconnectSuccessful = Super::HandleDisconnectCommand(Cmd, Ar, InWorld);
	if (bDisconnectSuccessful)
	{
		GotoState(SoldierGameInstanceState::MainMenu);
	}

	return bDisconnectSuccessful;
}

bool USoldierGameInstance::HandleTravelCommand(const TCHAR* Cmd, FOutputDevice& Ar, UWorld* InWorld)
{
	bool const bTravelSuccessful = Super::HandleTravelCommand(Cmd, Ar, InWorld);
	if (bTravelSuccessful)
	{
		GotoState(SoldierGameInstanceState::Playing);
	}

	return bTravelSuccessful;
}


void USoldierGameInstance::HandleSignInChangeMessaging()
{
	// Master user signed out, go to initial state (if we aren't there already)
	if ( CurrentState != GetInitialState() )
	{
#if SHOOTER_CONSOLE_UI
		// Display message on consoles
		const FText ReturnReason	= NSLOCTEXT( "ProfileMessages", "SignInChange", "Sign in status change occurred." );
		const FText OKButton		= NSLOCTEXT( "DialogButtons", "OKAY", "OK" );

		ShowMessageThenGotoState(ReturnReason, OKButton, FText::GetEmpty(), GetInitialState());
#else								
		GotoInitialState();
#endif
	}
}

void USoldierGameInstance::HandleUserLoginChanged(int32 GameUserIndex, ELoginStatus::Type PreviousLoginStatus, ELoginStatus::Type LoginStatus, const FUniqueNetId& UserId)
{
	// On Switch, accounts can play in LAN games whether they are signed in online or not. 
#if PLATFORM_SWITCH
	const bool bDowngraded = LoginStatus == ELoginStatus::NotLoggedIn || (GetOnlineMode() == EOnlineMode::Online && LoginStatus == ELoginStatus::UsingLocalProfile);
#else
	const bool bDowngraded = (LoginStatus == ELoginStatus::NotLoggedIn && GetOnlineMode() == EOnlineMode::Offline) || (LoginStatus != ELoginStatus::LoggedIn && GetOnlineMode() != EOnlineMode::Offline);
#endif

	UE_LOG( LogOnline, Log, TEXT( "HandleUserLoginChanged: bDownGraded: %i" ), (int)bDowngraded );

	TSharedPtr<GenericApplication> GenericApplication = FSlateApplication::Get().GetPlatformApplication();
	bIsLicensed = GenericApplication->ApplicationLicenseValid();

	// Find the local player associated with this unique net id
	ULocalPlayer * LocalPlayer = FindLocalPlayerFromUniqueNetId( UserId );

	LocalPlayerOnlineStatus[GameUserIndex] = LoginStatus;

	// If this user is signed out, but was previously signed in, punt to welcome (or remove splitscreen if that makes sense)
	if ( LocalPlayer != NULL )
	{
		if (bDowngraded)
		{
			UE_LOG( LogOnline, Log, TEXT( "HandleUserLoginChanged: Player logged out: %s" ), *UserId.ToString() );

			LabelPlayerAsQuitter(LocalPlayer);

			// Check to see if this was the master, or if this was a split-screen player on the client
			if ( LocalPlayer == GetFirstGamePlayer() || GetOnlineMode() != EOnlineMode::Offline )
			{
				HandleSignInChangeMessaging();
			}
			else
			{
				// Remove local split-screen players from the list
				RemoveExistingLocalPlayer( LocalPlayer );
			}
		}
	}
}

void USoldierGameInstance::HandleAppWillDeactivate()
{
	if (CurrentState == SoldierGameInstanceState::Playing)
	{
		// Just have the first player controller pause the game.
		UWorld* const GameWorld = GetWorld();
		if (GameWorld)
		{
			// protect against a second pause menu loading on top of an existing one if someone presses the Jewel / PS buttons.
			bool bNeedsPause = true;
			for (FConstControllerIterator It = GameWorld->GetControllerIterator(); It; ++It)
			{
				ASoldierPlayerController* Controller = Cast<ASoldierPlayerController>(*It);
				if (Controller && (Controller->IsPaused() || Controller->IsGameMenuVisible()))
				{
					bNeedsPause = false;
					break;
				}
			}

			if (bNeedsPause)
			{
				ASoldierPlayerController* const Controller = Cast<ASoldierPlayerController>(GameWorld->GetFirstPlayerController());
				if (Controller)
				{
					Controller->ShowInGameMenu();
				}
			}
		}
	}
}

void USoldierGameInstance::HandleAppSuspend()
{
	// Players will lose connection on resume. However it is possible the game will exit before we get a resume, so we must kick off round end events here.
	UE_LOG( LogOnline, Warning, TEXT( "USoldierGameInstance::HandleAppSuspend" ) );
	UWorld* const World = GetWorld(); 
	ASoldierGameState* const GameState = World != NULL ? World->GetGameState<ASoldierGameState>() : NULL;

	if ( CurrentState != SoldierGameInstanceState::None && CurrentState != GetInitialState() )
	{
		UE_LOG( LogOnline, Warning, TEXT( "USoldierGameInstance::HandleAppSuspend: Sending round end event for players" ) );

		// Send round end events for local players
		for (int i = 0; i < LocalPlayers.Num(); ++i)
		{
			ASoldierPlayerController* SoldierPC = Cast<ASoldierPlayerController>(LocalPlayers[i]->PlayerController);
			if (SoldierPC && GameState)
			{
				// Assuming you can't win if you quit early
				SoldierPC->ClientSendRoundEndEvent(false, GameState->ElapsedTime);
			}
		}
	}
}

void USoldierGameInstance::HandleAppResume()
{
	UE_LOG( LogOnline, Log, TEXT( "USoldierGameInstance::HandleAppResume" ) );

	if ( CurrentState != SoldierGameInstanceState::None && CurrentState != GetInitialState() )
	{
		UE_LOG( LogOnline, Warning, TEXT( "USoldierGameInstance::HandleAppResume: Attempting to sign out players" ) );

		for ( int32 i = 0; i < LocalPlayers.Num(); ++i )
		{
			if ( LocalPlayers[i]->GetCachedUniqueNetId().IsValid() && LocalPlayerOnlineStatus[i] == ELoginStatus::LoggedIn && !IsLocalPlayerOnline( LocalPlayers[i] ) )
			{
				UE_LOG( LogOnline, Log, TEXT( "USoldierGameInstance::HandleAppResume: Signed out during resume." ) );
				HandleSignInChangeMessaging();
				break;
			}
		}
	}
}

void USoldierGameInstance::HandleAppLicenseUpdate()
{
	TSharedPtr<GenericApplication> GenericApplication = FSlateApplication::Get().GetPlatformApplication();
	bIsLicensed = GenericApplication->ApplicationLicenseValid();
}

void USoldierGameInstance::HandleSafeFrameChanged()
{
	UCanvas::UpdateAllCanvasSafeZoneData();
}

void USoldierGameInstance::RemoveExistingLocalPlayer(ULocalPlayer* ExistingPlayer)
{
	check(ExistingPlayer);
	if (ExistingPlayer->PlayerController != NULL)
	{
		// Kill the player
		ASoldierCharacter* MyPawn = Cast<ASoldierCharacter>(ExistingPlayer->PlayerController->GetPawn());
		if ( MyPawn )
		{
			MyPawn->KilledBy(NULL);
		}
	}

	// Remove local split-screen players from the list
	RemoveLocalPlayer( ExistingPlayer );
}

void USoldierGameInstance::RemoveSplitScreenPlayers()
{
	// if we had been split screen, toss the extra players now
	// remove every player, back to front, except the first one
	while (LocalPlayers.Num() > 1)
	{
		ULocalPlayer* const PlayerToRemove = LocalPlayers.Last();
		RemoveExistingLocalPlayer(PlayerToRemove);
	}
}

FReply USoldierGameInstance::OnPairingUsePreviousProfile()
{
	// Do nothing (except hide the message) if they want to continue using previous profile
	USoldierGameViewportClient * SoldierViewport = Cast<USoldierGameViewportClient>(GetGameViewportClient());

	if ( SoldierViewport != nullptr )
	{
		SoldierViewport->HideDialog();
	}

	return FReply::Handled();
}

FReply USoldierGameInstance::OnPairingUseNewProfile()
{
	HandleSignInChangeMessaging();
	return FReply::Handled();
}

void USoldierGameInstance::HandleControllerPairingChanged( int GameUserIndex, const FUniqueNetId& PreviousUser, const FUniqueNetId& NewUser )
{
	UE_LOG(LogOnlineGame, Log, TEXT("USoldierGameInstance::HandleControllerPairingChanged GameUserIndex %d PreviousUser '%s' NewUser '%s'"),
		GameUserIndex, *PreviousUser.ToString(), *NewUser.ToString());
	
	if ( CurrentState == SoldierGameInstanceState::WelcomeScreen )
	{
		// Don't care about pairing changes at welcome screen
		return;
	}

#if SHOOTER_CONSOLE_UI && PLATFORM_XBOXONE
	if ( IgnorePairingChangeForControllerId != -1 && GameUserIndex == IgnorePairingChangeForControllerId )
	{
		// We were told to ignore
		IgnorePairingChangeForControllerId = -1;	// Reset now so there there is no chance this remains in a bad state
		return;
	}

	if ( PreviousUser.IsValid() && !NewUser.IsValid() )
	{
		// Treat this as a disconnect or signout, which is handled somewhere else
		return;
	}

	if ( !PreviousUser.IsValid() && NewUser.IsValid() )
	{
		// Treat this as a signin
		ULocalPlayer * ControlledLocalPlayer = FindLocalPlayerFromControllerId( GameUserIndex );

		if ( ControlledLocalPlayer != NULL && !ControlledLocalPlayer->GetCachedUniqueNetId().IsValid() )
		{
			// If a player that previously selected "continue without saving" signs into this controller, move them back to welcome screen
			HandleSignInChangeMessaging();
		}
		
		return;
	}

	// Find the local player currently being controlled by this controller
	ULocalPlayer * ControlledLocalPlayer	= FindLocalPlayerFromControllerId( GameUserIndex );

	// See if the newly assigned profile is in our local player list
	ULocalPlayer * NewLocalPlayer			= FindLocalPlayerFromUniqueNetId( NewUser );

	// If the local player being controlled is not the target of the pairing change, then give them a chance 
	// to continue controlling the old player with this controller
	if ( ControlledLocalPlayer != nullptr && ControlledLocalPlayer != NewLocalPlayer )
	{
		USoldierGameViewportClient * SoldierViewport = Cast<USoldierGameViewportClient>(GetGameViewportClient());

		if ( SoldierViewport != nullptr )
		{
			SoldierViewport->ShowDialog( 
				nullptr,
				ESoldierDialogType::Generic,
				NSLOCTEXT("ProfileMessages", "PairingChanged", "Your controller has been paired to another profile, would you like to switch to this new profile now? Selecting YES will sign out of the previous profile."),
				NSLOCTEXT("DialogButtons", "YES", "A - YES"),
				NSLOCTEXT("DialogButtons", "NO", "B - NO"),
				FOnClicked::CreateUObject(this, &USoldierGameInstance::OnPairingUseNewProfile),
				FOnClicked::CreateUObject(this, &USoldierGameInstance::OnPairingUsePreviousProfile)
			);
		}
	}
#endif
}

void USoldierGameInstance::HandleControllerConnectionChange( bool bIsConnection, int32 Unused, int32 GameUserIndex )
{
	UE_LOG(LogOnlineGame, Log, TEXT("USoldierGameInstance::HandleControllerConnectionChange bIsConnection %d GameUserIndex %d"),
		bIsConnection, GameUserIndex);

	if(!bIsConnection)
	{
		// Controller was disconnected

		// Find the local player associated with this user index
		ULocalPlayer * LocalPlayer = FindLocalPlayerFromControllerId( GameUserIndex );

		if ( LocalPlayer == NULL )
		{
			return;		// We don't care about players we aren't tracking
		}

		// Invalidate this local player's controller id.
		LocalPlayer->SetControllerId(-1);
	}
}

FReply USoldierGameInstance::OnControllerReconnectConfirm()
{
	USoldierGameViewportClient * SoldierViewport = Cast<USoldierGameViewportClient>(GetGameViewportClient());
	if(SoldierViewport)
	{
		SoldierViewport->HideDialog();
	}

	return FReply::Handled();
}

TSharedPtr< const FUniqueNetId > USoldierGameInstance::GetUniqueNetIdFromControllerId( const int ControllerId )
{
	IOnlineIdentityPtr OnlineIdentityInt = Online::GetIdentityInterface(GetWorld());

	if ( OnlineIdentityInt.IsValid() )
	{
		TSharedPtr<const FUniqueNetId> UniqueId = OnlineIdentityInt->GetUniquePlayerId( ControllerId );

		if ( UniqueId.IsValid() )
		{
			return UniqueId;
		}
	}

	return nullptr;
}

void USoldierGameInstance::SetOnlineMode(EOnlineMode InOnlineMode)
{
	OnlineMode = InOnlineMode;
	UpdateUsingMultiplayerFeatures(InOnlineMode == EOnlineMode::Online);
}

void USoldierGameInstance::UpdateUsingMultiplayerFeatures(bool bIsUsingMultiplayerFeatures)
{
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());

	if (OnlineSub)
	{
		for (int32 i = 0; i < LocalPlayers.Num(); ++i)
		{
			ULocalPlayer* LocalPlayer = LocalPlayers[i];

			FUniqueNetIdRepl PlayerId = LocalPlayer->GetPreferredUniqueNetId();
			if (PlayerId.IsValid())
			{
				OnlineSub->SetUsingMultiplayerFeatures(*PlayerId, bIsUsingMultiplayerFeatures);
			}
		}
	}
}

void USoldierGameInstance::TravelToSession(const FName& SessionName)
{
	// Added to handle failures when joining using quickmatch (handles issue of joining a game that just ended, i.e. during game ending timer)
	AddNetworkFailureHandlers();
	ShowLoadingScreen();
	GotoState(SoldierGameInstanceState::Playing);
	InternalTravelToSession(SessionName);
}

void USoldierGameInstance::SetIgnorePairingChangeForControllerId( const int32 ControllerId )
{
	IgnorePairingChangeForControllerId = ControllerId;
}

bool USoldierGameInstance::IsLocalPlayerOnline(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == NULL)
	{
		return false;
	}
	const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if(OnlineSub)
	{
		const IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
		if(IdentityInterface.IsValid())
		{
			FUniqueNetIdRepl UniqueId = LocalPlayer->GetCachedUniqueNetId();
			if (UniqueId.IsValid())
			{
				const ELoginStatus::Type LoginStatus = IdentityInterface->GetLoginStatus(*UniqueId);
				if(LoginStatus == ELoginStatus::LoggedIn)
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool USoldierGameInstance::IsLocalPlayerSignedIn(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == NULL)
	{
		return false;
	}

	const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		const IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
		if (IdentityInterface.IsValid())
		{
			FUniqueNetIdRepl UniqueId = LocalPlayer->GetCachedUniqueNetId();
			if (UniqueId.IsValid())
			{
				return true;
			}
		}
	}

	return false;
}

bool USoldierGameInstance::ValidatePlayerForOnlinePlay(ULocalPlayer* LocalPlayer)
{
	// Get the viewport
	USoldierGameViewportClient * SoldierViewport = Cast<USoldierGameViewportClient>(GetGameViewportClient());

#if PLATFORM_XBOXONE
	if (CurrentConnectionStatus != EOnlineServerConnectionStatus::Connected)
	{
		// Don't let them play online if they aren't connected to Xbox LIVE
		if (SoldierViewport != NULL)
		{
			const FText Msg				= NSLOCTEXT("NetworkFailures", "ServiceDisconnected", "You must be connected to the Xbox LIVE service to play online.");
			const FText OKButtonString	= NSLOCTEXT("DialogButtons", "OKAY", "OK");

			SoldierViewport->ShowDialog( 
				NULL,
				ESoldierDialogType::Generic,
				Msg,
				OKButtonString,
				FText::GetEmpty(),
				FOnClicked::CreateUObject(this, &USoldierGameInstance::OnConfirmGeneric),
				FOnClicked::CreateUObject(this, &USoldierGameInstance::OnConfirmGeneric)
			);
		}

		return false;
	}
#endif

	if (!IsLocalPlayerOnline(LocalPlayer))
	{
		// Don't let them play online if they aren't online
		if (SoldierViewport != NULL)
		{
			const FText Msg				= NSLOCTEXT("NetworkFailures", "MustBeSignedIn", "You must be signed in to play online");
			const FText OKButtonString	= NSLOCTEXT("DialogButtons", "OKAY", "OK");

			SoldierViewport->ShowDialog( 
				NULL,
				ESoldierDialogType::Generic,
				Msg,
				OKButtonString,
				FText::GetEmpty(),
				FOnClicked::CreateUObject(this, &USoldierGameInstance::OnConfirmGeneric),
				FOnClicked::CreateUObject(this, &USoldierGameInstance::OnConfirmGeneric)
			);
		}

		return false;
	}

	return true;
}

bool USoldierGameInstance::ValidatePlayerIsSignedIn(ULocalPlayer* LocalPlayer)
{
	// Get the viewport
	USoldierGameViewportClient * SoldierViewport = Cast<USoldierGameViewportClient>(GetGameViewportClient());

	if (!IsLocalPlayerSignedIn(LocalPlayer))
	{
		// Don't let them play online if they aren't online
		if (SoldierViewport != NULL)
		{
			const FText Msg = NSLOCTEXT("NetworkFailures", "MustBeSignedIn", "You must be signed in to play online");
			const FText OKButtonString = NSLOCTEXT("DialogButtons", "OKAY", "OK");

			SoldierViewport->ShowDialog(
				NULL,
				ESoldierDialogType::Generic,
				Msg,
				OKButtonString,
				FText::GetEmpty(),
				FOnClicked::CreateUObject(this, &USoldierGameInstance::OnConfirmGeneric),
				FOnClicked::CreateUObject(this, &USoldierGameInstance::OnConfirmGeneric)
			);
		}

		return false;
	}

	return true;
}


FReply USoldierGameInstance::OnConfirmGeneric()
{
	USoldierGameViewportClient * SoldierViewport = Cast<USoldierGameViewportClient>(GetGameViewportClient());
	if(SoldierViewport)
	{
		SoldierViewport->HideDialog();
	}

	return FReply::Handled();
}

void USoldierGameInstance::StartOnlinePrivilegeTask(const IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate& Delegate, EUserPrivileges::Type Privilege, TSharedPtr< const FUniqueNetId > UserId)
{
	WaitMessageWidget = SNew(SSoldierWaitDialog)
		.MessageText(NSLOCTEXT("NetworkStatus", "CheckingPrivilegesWithServer", "Checking privileges with server.  Please wait..."));

	if (GEngine && GEngine->GameViewport)
	{
		UGameViewportClient* const GVC = GEngine->GameViewport;
		GVC->AddViewportWidgetContent(WaitMessageWidget.ToSharedRef());
	}

	IOnlineIdentityPtr Identity = Online::GetIdentityInterface(GetWorld());
	if (Identity.IsValid() && UserId.IsValid())
	{		
		Identity->GetUserPrivilege(*UserId, Privilege, Delegate);
	}
	else
	{
		// Can only get away with faking the UniqueNetId here because the delegates don't use it
		Delegate.ExecuteIfBound(FUniqueNetIdString(), Privilege, (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures);
	}
}

void USoldierGameInstance::CleanupOnlinePrivilegeTask()
{
	if (GEngine && GEngine->GameViewport && WaitMessageWidget.IsValid())
	{
		UGameViewportClient* const GVC = GEngine->GameViewport;
		GVC->RemoveViewportWidgetContent(WaitMessageWidget.ToSharedRef());
	}
}

void USoldierGameInstance::DisplayOnlinePrivilegeFailureDialogs(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults)
{	
	// Show warning that the user cannot play due to age restrictions
	USoldierGameViewportClient * SoldierViewport = Cast<USoldierGameViewportClient>(GetGameViewportClient());
	TWeakObjectPtr<ULocalPlayer> OwningPlayer;
	if (GEngine)
	{
		for (auto It = GEngine->GetLocalPlayerIterator(GetWorld()); It; ++It)
		{
			FUniqueNetIdRepl OtherId = (*It)->GetPreferredUniqueNetId();
			if (OtherId.IsValid())
			{
				if (UserId == (*OtherId))
				{
					OwningPlayer = *It;
				}
			}
		}
	}
	
	if (SoldierViewport != NULL && OwningPlayer.IsValid())
	{
		if ((PrivilegeResults & (uint32)IOnlineIdentity::EPrivilegeResults::AccountTypeFailure) != 0)
		{
			IOnlineExternalUIPtr ExternalUI = Online::GetExternalUIInterface(GetWorld());
			if (ExternalUI.IsValid())
			{
				ExternalUI->ShowAccountUpgradeUI(UserId);
			}
		}
		else if ((PrivilegeResults & (uint32)IOnlineIdentity::EPrivilegeResults::RequiredSystemUpdate) != 0)
		{
			SoldierViewport->ShowDialog(
				OwningPlayer.Get(),
				ESoldierDialogType::Generic,
				NSLOCTEXT("OnlinePrivilegeResult", "RequiredSystemUpdate", "A required system update is available.  Please upgrade to access online features."),
				NSLOCTEXT("DialogButtons", "OKAY", "OK"),
				FText::GetEmpty(),
				FOnClicked::CreateUObject(this, &USoldierGameInstance::OnConfirmGeneric),
				FOnClicked::CreateUObject(this, &USoldierGameInstance::OnConfirmGeneric)
				);
		}
		else if ((PrivilegeResults & (uint32)IOnlineIdentity::EPrivilegeResults::RequiredPatchAvailable) != 0)
		{
			SoldierViewport->ShowDialog(
				OwningPlayer.Get(),
				ESoldierDialogType::Generic,
				NSLOCTEXT("OnlinePrivilegeResult", "RequiredPatchAvailable", "A required game patch is available.  Please upgrade to access online features."),
				NSLOCTEXT("DialogButtons", "OKAY", "OK"),
				FText::GetEmpty(),
				FOnClicked::CreateUObject(this, &USoldierGameInstance::OnConfirmGeneric),
				FOnClicked::CreateUObject(this, &USoldierGameInstance::OnConfirmGeneric)
				);
		}
		else if ((PrivilegeResults & (uint32)IOnlineIdentity::EPrivilegeResults::AgeRestrictionFailure) != 0)
		{
			SoldierViewport->ShowDialog(
				OwningPlayer.Get(),
				ESoldierDialogType::Generic,
				NSLOCTEXT("OnlinePrivilegeResult", "AgeRestrictionFailure", "Cannot play due to age restrictions!"),
				NSLOCTEXT("DialogButtons", "OKAY", "OK"),
				FText::GetEmpty(),
				FOnClicked::CreateUObject(this, &USoldierGameInstance::OnConfirmGeneric),
				FOnClicked::CreateUObject(this, &USoldierGameInstance::OnConfirmGeneric)
				);
		}
		else if ((PrivilegeResults & (uint32)IOnlineIdentity::EPrivilegeResults::UserNotFound) != 0)
		{
			SoldierViewport->ShowDialog(
				OwningPlayer.Get(),
				ESoldierDialogType::Generic,
				NSLOCTEXT("OnlinePrivilegeResult", "UserNotFound", "Cannot play due invalid user!"),
				NSLOCTEXT("DialogButtons", "OKAY", "OK"),
				FText::GetEmpty(),
				FOnClicked::CreateUObject(this, &USoldierGameInstance::OnConfirmGeneric),
				FOnClicked::CreateUObject(this, &USoldierGameInstance::OnConfirmGeneric)
				);
		}
		else if ((PrivilegeResults & (uint32)IOnlineIdentity::EPrivilegeResults::GenericFailure) != 0)
		{
			SoldierViewport->ShowDialog(
				OwningPlayer.Get(),
				ESoldierDialogType::Generic,
				NSLOCTEXT("OnlinePrivilegeResult", "GenericFailure", "Cannot play online.  Check your network connection."),
				NSLOCTEXT("DialogButtons", "OKAY", "OK"),
				FText::GetEmpty(),
				FOnClicked::CreateUObject(this, &USoldierGameInstance::OnConfirmGeneric),
				FOnClicked::CreateUObject(this, &USoldierGameInstance::OnConfirmGeneric)
				);
		}
	}
}

void USoldierGameInstance::OnRegisterLocalPlayerComplete(const FUniqueNetId& PlayerId, EOnJoinSessionCompleteResult::Type Result)
{
	FinishSessionCreation(Result);
}

void USoldierGameInstance::FinishSessionCreation(EOnJoinSessionCompleteResult::Type Result)
{
	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		// This will send any Play Together invites if necessary, or do nothing.
		SendPlayTogetherInvites();

		// Travel to the specified match URL
		GetWorld()->ServerTravel(TravelURL);
	}
	else
	{
		FText ReturnReason = NSLOCTEXT("NetworkErrors", "CreateSessionFailed", "Failed to create session.");
		FText OKButton = NSLOCTEXT("DialogButtons", "OKAY", "OK");
		ShowMessageThenGoMain(ReturnReason, OKButton, FText::GetEmpty());
	}
}

FString USoldierGameInstance::GetQuickMatchUrl()
{
	static const FString QuickMatchUrl(TEXT("/Game/Maps/Highrise?game=TDM?listen"));
	return QuickMatchUrl;
}

void USoldierGameInstance::BeginHostingQuickMatch()
{
	ShowLoadingScreen();
	GotoState(SoldierGameInstanceState::Playing);

	// Travel to the specified match URL
	GetWorld()->ServerTravel(GetQuickMatchUrl());	
}

void USoldierGameInstance::OnPlayTogetherEventReceived(const int32 UserIndex, const TArray<TSharedPtr<const FUniqueNetId>>& UserIdList)
{
	PlayTogetherInfo = FSoldierPlayTogetherInfo(UserIndex, UserIdList);

	const IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetWorld());
	check(OnlineSub);

	const IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
	check(SessionInterface.IsValid());

	// If we have available slots to accomedate the whole party in our current sessions, we should send invites to the existing one
	// instead of a new one according to Sony's best practices.
	const FNamedOnlineSession* const Session = SessionInterface->GetNamedSession(NAME_GameSession);
	if (Session != nullptr && Session->NumOpenPrivateConnections + Session->NumOpenPublicConnections >= UserIdList.Num())
	{
		SendPlayTogetherInvites();
	}
	// Always handle Play Together in the main menu since the player has session customization options.
	else if (CurrentState == SoldierGameInstanceState::MainMenu)
	{
		MainMenuUI->OnPlayTogetherEventReceived();
	}
	else if (CurrentState == SoldierGameInstanceState::WelcomeScreen)
	{
		StartOnlinePrivilegeTask(IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateUObject(this, &USoldierGameInstance::OnUserCanPlayTogether), EUserPrivileges::CanPlayOnline, PendingInvite.UserId);
	}
	else
	{
		GotoState(SoldierGameInstanceState::MainMenu);
	}
}

void USoldierGameInstance::SendPlayTogetherInvites()
{
	const IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetWorld());
	check(OnlineSub);

	const IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
	check(SessionInterface.IsValid());

	if (PlayTogetherInfo.UserIndex != -1)
	{
		for (const ULocalPlayer* LocalPlayer : LocalPlayers)
		{
			if (LocalPlayer->GetControllerId() == PlayTogetherInfo.UserIndex)
			{
				FUniqueNetIdRepl PlayerId = LocalPlayer->GetPreferredUniqueNetId();
				if (PlayerId.IsValid())
				{
					// Automatically send invites to friends in the player's PS4 party to conform with Play Together requirements
					for (const TSharedPtr<const FUniqueNetId>& FriendId : PlayTogetherInfo.UserIdList)
					{
						SessionInterface->SendSessionInviteToFriend(*PlayerId, NAME_GameSession, *FriendId.ToSharedRef());
					}
				}

			}
		}

		PlayTogetherInfo = FSoldierPlayTogetherInfo();
	}
}

void USoldierGameInstance::ReceivedNetworkEncryptionToken(const FString& EncryptionToken, const FOnEncryptionKeyResponse& Delegate)
{
	// This is a simple implementation to demonstrate using encryption for game traffic using a hardcoded key.
	// For a complete implementation, you would likely want to retrieve the encryption key from a secure source,
	// such as from a web service over HTTPS. This could be done in this function, even asynchronously - just
	// call the response delegate passed in once the key is known. The contents of the EncryptionToken is up to the user,
	// but it will generally contain information used to generate a unique encryption key, such as a user and/or session ID.

	FEncryptionKeyResponse Response(EEncryptionResponse::Failure, TEXT("Unknown encryption failure"));

	if (EncryptionToken.IsEmpty())
	{
		Response.Response = EEncryptionResponse::InvalidToken;
		Response.ErrorMsg = TEXT("Encryption token is empty.");
	}
	else
	{
		Response.Response = EEncryptionResponse::Success;
		Response.EncryptionData.Key = DebugTestEncryptionKey;
	}

	Delegate.ExecuteIfBound(Response);

}

void USoldierGameInstance::ReceivedNetworkEncryptionAck(const FOnEncryptionKeyResponse& Delegate)
{
	// This is a simple implementation to demonstrate using encryption for game traffic using a hardcoded key.
	// For a complete implementation, you would likely want to retrieve the encryption key from a secure source,
	// such as from a web service over HTTPS. This could be done in this function, even asynchronously - just
	// call the response delegate passed in once the key is known.

	FEncryptionKeyResponse Response;
	Response.Response = EEncryptionResponse::Success;
	Response.EncryptionData.Key = DebugTestEncryptionKey;

	Delegate.ExecuteIfBound(Response);
}

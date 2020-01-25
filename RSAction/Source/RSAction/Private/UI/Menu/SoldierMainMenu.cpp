// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "RSAction.h"
#include "SoldierMainMenu.h"
#include "SoldierGameLoadingScreen.h"
#include "SoldierStyle.h"
#include "SoldierMenuSoundsWidgetStyle.h"
#include "SoldierGameInstance.h"
#include "SlateBasics.h"
#include "SlateExtras.h"
#include "GenericPlatformChunkInstall.h"
#include "Online/SoldierOnlineGameSettings.h"
#include "OnlineSubsystemSessionSettings.h"
#include "SSoldierConfirmationDialog.h"
#include "SoldierMenuItemWidgetStyle.h"
#include "SoldierGameUserSettings.h"
#include "SoldierGameViewportClient.h"
#include "SoldierPersistentUser.h"
#include "Player/SoldierLocalPlayer.h"
#include "OnlineSubsystemUtils.h"

#define LOCTEXT_NAMESPACE "RSAction.HUD.Menu"

#define MAX_BOT_COUNT 8

static const FString MapNames[] = { TEXT("DesertTownDemo"), TEXT("DesertTownDemo_1217") };
static const FString JoinMapNames[] = { TEXT("Any"), TEXT("DesertTownDemo"), TEXT("DesertTownDemo_1217") };
static const FName PackageNames[] = { TEXT("DesertTownDemo_1217.umap"), TEXT("DesertTownDemo_1217.umap") };
static const int DefaultTDMMap = 1;
static const int DefaultFFAMap = 0; 
static const float QuickmatchUIAnimationTimeDuration = 30.f;

//use an EMap index, get back the ChunkIndex that map should be part of.
//Instead of this mapping we should really use the AssetRegistry to query for chunk mappings, but maps aren't members of the AssetRegistry yet.
static const int ChunkMapping[] = { 1, 2 };

#if PLATFORM_SWITCH
#	define LOGIN_REQUIRED_FOR_ONLINE_PLAY 1
#else
#	define LOGIN_REQUIRED_FOR_ONLINE_PLAY 0
#endif

#if PLATFORM_SWITCH
#	define CONSOLE_LAN_SUPPORTED 1
#else
#	define CONSOLE_LAN_SUPPORTED 0
#endif

FSoldierMainMenu::~FSoldierMainMenu()
{
	CleanupOnlinePrivilegeTask();
}

void FSoldierMainMenu::Construct(TWeakObjectPtr<USoldierGameInstance> _GameInstance, TWeakObjectPtr<ULocalPlayer> _PlayerOwner)
{
	bShowingDownloadPct = false;
	bAnimateQuickmatchSearchingUI = false;
	bUsedInputToCancelQuickmatchSearch = false;
	bQuickmatchSearchRequestCanceled = false;
	bIncQuickMAlpha = false;
	PlayerOwner = _PlayerOwner;
	MatchType = EMatchType::Custom;

	check(_GameInstance.IsValid());

	GameInstance = _GameInstance;
	PlayerOwner = _PlayerOwner;

	OnCancelMatchmakingCompleteDelegate = FOnCancelMatchmakingCompleteDelegate::CreateSP(this, &FSoldierMainMenu::OnCancelMatchmakingComplete);
	OnMatchmakingCompleteDelegate = FOnMatchmakingCompleteDelegate::CreateSP(this, &FSoldierMainMenu::OnMatchmakingComplete);
	
	// read user settings
#if SHOOTER_CONSOLE_UI
	bIsLanMatch = FParse::Param(FCommandLine::Get(), TEXT("forcelan"));
#else
	USoldierGameUserSettings* const UserSettings = CastChecked<USoldierGameUserSettings>(GEngine->GetGameUserSettings());
	bIsLanMatch = UserSettings->IsLanMatch();
	bIsDedicatedServer = UserSettings->IsDedicatedServer();
#endif

	BotsCountOpt = 1;
	bIsRecordingDemo = false;

	if(GetPersistentUser())
	{
		BotsCountOpt = GetPersistentUser()->GetBotsCount();
		bIsRecordingDemo = GetPersistentUser()->IsRecordingDemos();
	}		

	// number entries 0 up to MAX_BOX_COUNT
	TArray<FText> BotsCountList;
	for (int32 i = 0; i <= MAX_BOT_COUNT; i++)
	{
		BotsCountList.Add(FText::AsNumber(i));
	}
	
	TArray<FText> MapList;
	for (int32 i = 0; i < UE_ARRAY_COUNT(MapNames); ++i)
	{
		MapList.Add(FText::FromString(MapNames[i]));		
	}

	TArray<FText> JoinMapList;
	for (int32 i = 0; i < UE_ARRAY_COUNT(JoinMapNames); ++i)
	{
		JoinMapList.Add(FText::FromString(JoinMapNames[i]));
	}

	TArray<FText> OnOffList;
	OnOffList.Add( LOCTEXT("Off","OFF") );
	OnOffList.Add( LOCTEXT("On","ON") );

	SoldierOptions = MakeShareable(new FSoldierOptions()); 
	SoldierOptions->Construct(GetPlayerOwner());
	SoldierOptions->TellInputAboutKeybindings();
	SoldierOptions->OnApplyChanges.BindSP(this, &FSoldierMainMenu::CloseSubMenu);

	//Now that we are here, build our menu 
	MenuWidget.Reset();
	MenuWidgetContainer.Reset();

	TArray<FString> Keys;
	GConfig->GetSingleLineArray(TEXT("/Script/SwitchRuntimeSettings.SwitchRuntimeSettings"), TEXT("LeaderboardMap"), Keys, GEngineIni);

	if (GEngine && GEngine->GameViewport)
	{		
		SAssignNew(MenuWidget, SSoldierMenuWidget)
			.Cursor(EMouseCursor::Default)
			.PlayerOwner(GetPlayerOwner())
			.IsGameMenu(false);

		SAssignNew(MenuWidgetContainer, SWeakWidget)
			.PossiblyNullContent(MenuWidget);		

		TSharedPtr<FSoldierMenuItem> RootMenuItem;

				
		SAssignNew(SplitScreenLobbyWidget, SSoldierSplitScreenLobby)
			.PlayerOwner(GetPlayerOwner())
			.OnCancelClicked(FOnClicked::CreateSP(this, &FSoldierMainMenu::OnSplitScreenBackedOut)) 
			.OnPlayClicked(FOnClicked::CreateSP(this, &FSoldierMainMenu::OnSplitScreenPlay));

		FText Msg = LOCTEXT("No matches could be found", "No matches could be found");
		FText OKButtonString = NSLOCTEXT("DialogButtons", "OKAY", "OK");
		QuickMatchFailureWidget = SNew(SSoldierConfirmationDialog).PlayerOwner(PlayerOwner)			
			.MessageText(Msg)
			.ConfirmText(OKButtonString)
			.CancelText(FText())
			.OnConfirmClicked(FOnClicked::CreateRaw(this, &FSoldierMainMenu::OnQuickMatchFailureUICancel))
			.OnCancelClicked(FOnClicked::CreateRaw(this, &FSoldierMainMenu::OnQuickMatchFailureUICancel));

		Msg = LOCTEXT("Searching for Match...", "SEARCHING FOR MATCH...");
		OKButtonString = LOCTEXT("Stop", "STOP");
		QuickMatchSearchingWidget = SNew(SSoldierConfirmationDialog).PlayerOwner(PlayerOwner)			
			.MessageText(Msg)
			.ConfirmText(OKButtonString)
			.CancelText(FText())
			.OnConfirmClicked(FOnClicked::CreateRaw(this, &FSoldierMainMenu::OnQuickMatchSearchingUICancel))
			.OnCancelClicked(FOnClicked::CreateRaw(this, &FSoldierMainMenu::OnQuickMatchSearchingUICancel));

		SAssignNew(SplitScreenLobbyWidgetContainer, SWeakWidget)
			.PossiblyNullContent(SplitScreenLobbyWidget);		

		SAssignNew(QuickMatchFailureWidgetContainer, SWeakWidget)
			.PossiblyNullContent(QuickMatchFailureWidget);	

		SAssignNew(QuickMatchSearchingWidgetContainer, SWeakWidget)
			.PossiblyNullContent(QuickMatchSearchingWidget);

		FText StoppingOKButtonString = LOCTEXT("Stopping", "STOPPING...");
		QuickMatchStoppingWidget = SNew(SSoldierConfirmationDialog).PlayerOwner(PlayerOwner)			
			.MessageText(Msg)
			.ConfirmText(StoppingOKButtonString)
			.CancelText(FText())
			.OnConfirmClicked(FOnClicked())
			.OnCancelClicked(FOnClicked());

		SAssignNew(QuickMatchStoppingWidgetContainer, SWeakWidget)
			.PossiblyNullContent(QuickMatchStoppingWidget);

#if PLATFORM_XBOXONE
		TSharedPtr<FSoldierMenuItem> MenuItem;

		// HOST ONLINE menu option
		{
			MenuItem = MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("HostCustom", "HOST CUSTOM"), this, &FSoldierMainMenu::OnHostOnlineSelected);

			// submenu under "HOST ONLINE"
			MenuHelper::AddMenuItemSP(MenuItem, LOCTEXT("TDMLong", "TEAM DEATHMATCH"), this, &FSoldierMainMenu::OnSplitScreenSelected);

			TSharedPtr<FSoldierMenuItem> NumberOfBotsOption = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("NumberOfBots", "NUMBER OF BOTS"), BotsCountList, this, &FSoldierMainMenu::BotCountOptionChanged);				
			NumberOfBotsOption->SelectedMultiChoice = BotsCountOpt;																

			HostOnlineMapOption = MenuHelper::AddMenuOption(MenuItem, LOCTEXT("SELECTED_LEVEL", "Map"), MapList);
		}

		// JOIN menu option
		{
			// JOIN menu option
			MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("FindCustom", "FIND CUSTOM"), this, &FSoldierMainMenu::OnJoinServer);

			// Server list widget that will be called up if appropriate
			MenuHelper::AddCustomMenuItem(JoinServerItem,SAssignNew(ServerListWidget,SSoldierServerList).OwnerWidget(MenuWidget).PlayerOwner(GetPlayerOwner()));
		}

		// QUICK MATCH menu option
		{
			MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("QuickMatch", "QUICK MATCH"), this, &FSoldierMainMenu::OnQuickMatchSelected);
		}

		// HOST OFFLINE menu option
		{
			MenuItem = MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("PlayOffline", "PLAY OFFLINE"),this, &FSoldierMainMenu::OnHostOfflineSelected);

			// submenu under "HOST OFFLINE"
			MenuHelper::AddMenuItemSP(MenuItem, LOCTEXT("TDMLong", "TEAM DEATHMATCH"), this, &FSoldierMainMenu::OnSplitScreenSelected);

			TSharedPtr<FSoldierMenuItem> NumberOfBotsOption = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("NumberOfBots", "NUMBER OF BOTS"), BotsCountList, this, &FSoldierMainMenu::BotCountOptionChanged);				
			NumberOfBotsOption->SelectedMultiChoice = BotsCountOpt;																

			HostOfflineMapOption = MenuHelper::AddMenuOption(MenuItem, LOCTEXT("SELECTED_LEVEL", "Map"), MapList);
		}
#elif SHOOTER_CONSOLE_UI
		TSharedPtr<FSoldierMenuItem> MenuItem;

		// HOST ONLINE menu option
		{
			HostOnlineMenuItem = MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("HostOnline", "HOST ONLINE"), this, &FSoldierMainMenu::OnHostOnlineSelected);

			// submenu under "HOST ONLINE"
#if LOGIN_REQUIRED_FOR_ONLINE_PLAY
			MenuHelper::AddMenuItemSP(HostOnlineMenuItem, LOCTEXT("TDMLong", "TEAM DEATHMATCH"), this, &FSoldierMainMenu::OnSplitScreenSelectedHostOnlineLoginRequired);
#else
			MenuHelper::AddMenuItemSP(HostOnlineMenuItem, LOCTEXT("TDMLong", "TEAM DEATHMATCH"), this, &FSoldierMainMenu::OnSplitScreenSelectedHostOnline);
#endif

			TSharedPtr<FSoldierMenuItem> NumberOfBotsOption = MenuHelper::AddMenuOptionSP(HostOnlineMenuItem, LOCTEXT("NumberOfBots", "NUMBER OF BOTS"), BotsCountList, this, &FSoldierMainMenu::BotCountOptionChanged);
			NumberOfBotsOption->SelectedMultiChoice = BotsCountOpt;																

			HostOnlineMapOption = MenuHelper::AddMenuOption(HostOnlineMenuItem, LOCTEXT("SELECTED_LEVEL", "Map"), MapList);
#if CONSOLE_LAN_SUPPORTED
			HostLANItem = MenuHelper::AddMenuOptionSP(HostOnlineMenuItem, LOCTEXT("LanMatch", "LAN"), OnOffList, this, &FSoldierMainMenu::LanMatchChanged);
			HostLANItem->SelectedMultiChoice = bIsLanMatch;
#endif
		}

		// HOST OFFLINE menu option
		{
			MenuItem = MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("HostOffline", "HOST OFFLINE"),this, &FSoldierMainMenu::OnHostOfflineSelected);

			// submenu under "HOST OFFLINE"
			MenuHelper::AddMenuItemSP(MenuItem, LOCTEXT("TDMLong", "TEAM DEATHMATCH"), this, &FSoldierMainMenu::OnSplitScreenSelected);

			TSharedPtr<FSoldierMenuItem> NumberOfBotsOption = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("NumberOfBots", "NUMBER OF BOTS"), BotsCountList, this, &FSoldierMainMenu::BotCountOptionChanged);				
			NumberOfBotsOption->SelectedMultiChoice = BotsCountOpt;																

			HostOfflineMapOption = MenuHelper::AddMenuOption(MenuItem, LOCTEXT("SELECTED_LEVEL", "Map"), MapList);
		}

		// QUICK MATCH menu option
		{
#if LOGIN_REQUIRED_FOR_ONLINE_PLAY
			MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("QuickMatch", "QUICK MATCH"), this, &FSoldierMainMenu::OnQuickMatchSelectedLoginRequired);
#else
			MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("QuickMatch", "QUICK MATCH"), this, &FSoldierMainMenu::OnQuickMatchSelected);
#endif
		}

		// JOIN menu option
		{
			// JOIN menu option
			MenuItem = MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("Join", "JOIN"), this, &FSoldierMainMenu::OnJoinSelected);

			// submenu under "join"
#if LOGIN_REQUIRED_FOR_ONLINE_PLAY
			MenuHelper::AddMenuItemSP(MenuItem, LOCTEXT("Server", "SERVER"), this, &FSoldierMainMenu::OnJoinServerLoginRequired);
#else
			MenuHelper::AddMenuItemSP(MenuItem, LOCTEXT("Server", "SERVER"), this, &FSoldierMainMenu::OnJoinServer);
#endif
			JoinMapOption = MenuHelper::AddMenuOption(MenuItem, LOCTEXT("SELECTED_LEVEL", "Map"), JoinMapList);

			// Server list widget that will be called up if appropriate
			MenuHelper::AddCustomMenuItem(JoinServerItem,SAssignNew(ServerListWidget,SSoldierServerList).OwnerWidget(MenuWidget).PlayerOwner(GetPlayerOwner()));

#if CONSOLE_LAN_SUPPORTED
			JoinLANItem = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("LanMatch", "LAN"), OnOffList, this, &FSoldierMainMenu::LanMatchChanged);
			JoinLANItem->SelectedMultiChoice = bIsLanMatch;
#endif
		}

#else
		TSharedPtr<FSoldierMenuItem> MenuItem;
		// HOST menu option
		MenuItem = MenuHelper::AddMenuItem(RootMenuItem, LOCTEXT("Host", "HOST"));

		// submenu under "host"
		MenuHelper::AddMenuItemSP(MenuItem, LOCTEXT("FFALong", "FREE FOR ALL"), this, &FSoldierMainMenu::OnUIHostFreeForAll);
		MenuHelper::AddMenuItemSP(MenuItem, LOCTEXT("TDMLong", "TEAM DEATHMATCH"), this, &FSoldierMainMenu::OnUIHostTeamDeathMatch);

		TSharedPtr<FSoldierMenuItem> NumberOfBotsOption = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("NumberOfBots", "NUMBER OF BOTS"), BotsCountList, this, &FSoldierMainMenu::BotCountOptionChanged);
		NumberOfBotsOption->SelectedMultiChoice = BotsCountOpt;

		HostOnlineMapOption = MenuHelper::AddMenuOption(MenuItem, LOCTEXT("SELECTED_LEVEL", "Map"), MapList);

		HostLANItem = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("LanMatch", "LAN"), OnOffList, this, &FSoldierMainMenu::LanMatchChanged);
		HostLANItem->SelectedMultiChoice = bIsLanMatch;

		RecordDemoItem = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("RecordDemo", "Record Demo"), OnOffList, this, &FSoldierMainMenu::RecordDemoChanged);
		RecordDemoItem->SelectedMultiChoice = bIsRecordingDemo;

		// JOIN menu option
		MenuItem = MenuHelper::AddMenuItem(RootMenuItem, LOCTEXT("Join", "JOIN"));

		// submenu under "join"
		MenuHelper::AddMenuItemSP(MenuItem, LOCTEXT("Server", "SERVER"), this, &FSoldierMainMenu::OnJoinServer);
		JoinLANItem = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("LanMatch", "LAN"), OnOffList, this, &FSoldierMainMenu::LanMatchChanged);
		JoinLANItem->SelectedMultiChoice = bIsLanMatch;

		DedicatedItem = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("Dedicated", "Dedicated"), OnOffList, this, &FSoldierMainMenu::DedicatedServerChanged);
		DedicatedItem->SelectedMultiChoice = bIsDedicatedServer;

		// Server list widget that will be called up if appropriate
		MenuHelper::AddCustomMenuItem(JoinServerItem,SAssignNew(ServerListWidget,SSoldierServerList).OwnerWidget(MenuWidget).PlayerOwner(GetPlayerOwner()));
#endif

		// Leaderboards
		MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("Leaderboards", "LEADERBOARDS"), this, &FSoldierMainMenu::OnShowLeaderboard);
		MenuHelper::AddCustomMenuItem(LeaderboardItem,SAssignNew(LeaderboardWidget,SSoldierLeaderboard).OwnerWidget(MenuWidget).PlayerOwner(GetPlayerOwner()));

		// Purchases
		MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("Store", "ONLINE STORE"), this, &FSoldierMainMenu::OnShowOnlineStore);
		MenuHelper::AddCustomMenuItem(OnlineStoreItem, SAssignNew(OnlineStoreWidget, SSoldierOnlineStore).OwnerWidget(MenuWidget).PlayerOwner(GetPlayerOwner()));

#if !SHOOTER_CONSOLE_UI

		// Demos
		{
			MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("Demos", "DEMOS"), this, &FSoldierMainMenu::OnShowDemoBrowser);
			MenuHelper::AddCustomMenuItem(DemoBrowserItem,SAssignNew(DemoListWidget,SSoldierDemoList).OwnerWidget(MenuWidget).PlayerOwner(GetPlayerOwner()));
		}
#endif

		// Options
		MenuHelper::AddExistingMenuItem(RootMenuItem, SoldierOptions->OptionsItem.ToSharedRef());

		if(FSlateApplication::Get().SupportsSystemHelp())
		{
			TSharedPtr<FSoldierMenuItem> HelpSubMenu = MenuHelper::AddMenuItem(RootMenuItem, LOCTEXT("Help", "HELP"));
			HelpSubMenu->OnConfirmMenuItem.BindStatic([](){ FSlateApplication::Get().ShowSystemHelp(); });
		}

		// QUIT option (for PC)
#if !SHOOTER_CONSOLE_UI
		MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("Quit", "QUIT"), this, &FSoldierMainMenu::OnUIQuit);
#endif

		MenuWidget->CurrentMenuTitle = LOCTEXT("MainMenu","MAIN ²Ëµ¥");
		MenuWidget->OnGoBack.BindSP(this, &FSoldierMainMenu::OnMenuGoBack);
		MenuWidget->MainMenu = MenuWidget->CurrentMenu = RootMenuItem->SubMenu;
		MenuWidget->OnMenuHidden.BindSP(this, &FSoldierMainMenu::OnMenuHidden);

		SoldierOptions->UpdateOptions();
		MenuWidget->BuildAndShowMenu();
	}
}

void FSoldierMainMenu::AddMenuToGameViewport()
{
	if (GEngine && GEngine->GameViewport)
	{
		UGameViewportClient* GVC = GEngine->GameViewport;
		GVC->AddViewportWidgetContent(MenuWidgetContainer.ToSharedRef());
		GVC->SetCaptureMouseOnClick(EMouseCaptureMode::NoCapture);
	}
}

void FSoldierMainMenu::RemoveMenuFromGameViewport()
{
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(MenuWidgetContainer.ToSharedRef());
	}
}

void FSoldierMainMenu::Tick(float DeltaSeconds)
{
	if (bAnimateQuickmatchSearchingUI)
	{
		FLinearColor QuickMColor = QuickMatchSearchingWidget->GetColorAndOpacity();
		if (bIncQuickMAlpha)
		{
			if (QuickMColor.A >= 1.f)
			{
				bIncQuickMAlpha = false;
			}
			else
			{
				QuickMColor.A += DeltaSeconds;
			}
		}
		else
		{
			if (QuickMColor.A <= .1f)
			{
				bIncQuickMAlpha = true;
			}
			else
			{
				QuickMColor.A -= DeltaSeconds;
			}
		}
		QuickMatchSearchingWidget->SetColorAndOpacity(QuickMColor);
		QuickMatchStoppingWidget->SetColorAndOpacity(QuickMColor);
	}

	IPlatformChunkInstall* ChunkInstaller = FPlatformMisc::GetPlatformChunkInstall();
	if (ChunkInstaller)
	{
		EMap SelectedMap = GetSelectedMap();
		// use assetregistry when maps are added to it.
		int32 MapChunk = ChunkMapping[(int)SelectedMap];
		EChunkLocation::Type ChunkLocation = ChunkInstaller->GetChunkLocation(MapChunk);

		FText UpdatedText;
		bool bUpdateText = false;
		if (ChunkLocation == EChunkLocation::NotAvailable)
		{			
			float PercentComplete = FMath::Min(ChunkInstaller->GetChunkProgress(MapChunk, EChunkProgressReportingType::PercentageComplete), 100.0f);									
			UpdatedText = FText::FromString(FString::Printf(TEXT("%s %4.0f%%"),*LOCTEXT("SELECTED_LEVEL", "Map").ToString(), PercentComplete));
			bUpdateText = true;
			bShowingDownloadPct = true;
		}
		else if (bShowingDownloadPct)
		{
			UpdatedText = LOCTEXT("SELECTED_LEVEL", "Map");			
			bUpdateText = true;
			bShowingDownloadPct = false;			
		}

		if (bUpdateText)
		{
			if (GameInstance.IsValid() && GameInstance->GetOnlineMode() != EOnlineMode::Offline && HostOnlineMapOption.IsValid())
			{
				HostOnlineMapOption->SetText(UpdatedText);
			}
			else if (HostOfflineMapOption.IsValid())
			{
				HostOfflineMapOption->SetText(UpdatedText);
			}
		}
	}
}

TStatId FSoldierMainMenu::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FSoldierMainMenu, STATGROUP_Tickables);
}

void FSoldierMainMenu::OnMenuHidden()
{	
#if SHOOTER_CONSOLE_UI
	// Menu was hidden from the top-level main menu, on consoles show the welcome screen again.
	if ( ensure(GameInstance.IsValid()))
	{
		GameInstance->GotoState(SoldierGameInstanceState::WelcomeScreen);
	}
#else
	RemoveMenuFromGameViewport();
#endif
}

void FSoldierMainMenu::OnQuickMatchSelectedLoginRequired()
{
	IOnlineIdentityPtr Identity = Online::GetIdentityInterface(GetTickableGameObjectWorld());
	if (Identity.IsValid())
	{
		int32 ControllerId = GetPlayerOwner()->GetControllerId();

		OnLoginCompleteDelegateHandle = Identity->AddOnLoginCompleteDelegate_Handle(ControllerId, FOnLoginCompleteDelegate::CreateRaw(this, &FSoldierMainMenu::OnLoginCompleteQuickmatch));
		Identity->Login(ControllerId, FOnlineAccountCredentials());
	}
}

void FSoldierMainMenu::OnLoginCompleteQuickmatch(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	IOnlineIdentityPtr Identity = Online::GetIdentityInterface(GetTickableGameObjectWorld());
	if (Identity.IsValid())
	{
		Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, OnLoginCompleteDelegateHandle);
	}

	OnQuickMatchSelected();
}

void FSoldierMainMenu::OnQuickMatchSelected()
{
	bQuickmatchSearchRequestCanceled = false;
#if SHOOTER_CONSOLE_UI
	if ( !ValidatePlayerForOnlinePlay(GetPlayerOwner()) )
	{
		return;
	}
#endif

	StartOnlinePrivilegeTask(IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateSP(this, &FSoldierMainMenu::OnUserCanPlayOnlineQuickMatch));
}

void FSoldierMainMenu::OnUserCanPlayOnlineQuickMatch(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults)
{
	CleanupOnlinePrivilegeTask();
	MenuWidget->LockControls(false);
	if (PrivilegeResults == (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures)
	{
		if (GameInstance.IsValid())
		{
			GameInstance->SetOnlineMode(EOnlineMode::Online);
		}

		MatchType = EMatchType::Quick;

		SplitScreenLobbyWidget->SetIsJoining(false);

		// Skip splitscreen for PS4
#if PLATFORM_PS4 || MAX_LOCAL_PLAYERS == 1
		BeginQuickMatchSearch();
#else
		UGameViewportClient* const GVC = GEngine->GameViewport;

		RemoveMenuFromGameViewport();
		GVC->AddViewportWidgetContent(SplitScreenLobbyWidgetContainer.ToSharedRef());

		SplitScreenLobbyWidget->Clear();
		FSlateApplication::Get().SetKeyboardFocus(SplitScreenLobbyWidget);
#endif
	}
	else if (GameInstance.IsValid())
	{

		GameInstance->DisplayOnlinePrivilegeFailureDialogs(UserId, Privilege, PrivilegeResults);
	}
}

FReply FSoldierMainMenu::OnConfirmGeneric()
{
	USoldierGameViewportClient* SoldierViewport = Cast<USoldierGameViewportClient>(GameInstance->GetGameViewportClient());
	if (SoldierViewport)
	{
		SoldierViewport->HideDialog();
	}

	return FReply::Handled();
}

void FSoldierMainMenu::BeginQuickMatchSearch()
{
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(GetTickableGameObjectWorld());
	if(!Sessions.IsValid())
	{
		UE_LOG(LogOnline, Warning, TEXT("Quick match is not supported: couldn't find online session interface."));
		return;
	}

	if (GetPlayerOwnerControllerId() == -1)
	{
		UE_LOG(LogOnline, Warning, TEXT("Quick match is not supported: Could not get controller id from player owner"));
		return;
	}

	QuickMatchSearchSettings = MakeShareable(new FSoldierOnlineSearchSettings(false, true));
	QuickMatchSearchSettings->QuerySettings.Set(SEARCH_XBOX_LIVE_HOPPER_NAME, FString("TeamDeathmatch"), EOnlineComparisonOp::Equals);
	QuickMatchSearchSettings->QuerySettings.Set(SEARCH_XBOX_LIVE_SESSION_TEMPLATE_NAME, FString("MatchSession"), EOnlineComparisonOp::Equals);
	QuickMatchSearchSettings->TimeoutInSeconds = 120.0f;

	FSoldierOnlineSessionSettings SessionSettings(false, true, 8);
	SessionSettings.Set(SETTING_GAMEMODE, FString("TDM"), EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(SETTING_MATCHING_HOPPER, FString("TeamDeathmatch"), EOnlineDataAdvertisementType::DontAdvertise);
	SessionSettings.Set(SETTING_MATCHING_TIMEOUT, 120.0f, EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(SETTING_SESSION_TEMPLATE_NAME, FString("GameSession"), EOnlineDataAdvertisementType::DontAdvertise);

	TSharedRef<FOnlineSessionSearch> QuickMatchSearchSettingsRef = QuickMatchSearchSettings.ToSharedRef();
	
	DisplayQuickmatchSearchingUI();

	Sessions->ClearOnMatchmakingCompleteDelegate_Handle(OnMatchmakingCompleteDelegateHandle);
	OnMatchmakingCompleteDelegateHandle = Sessions->AddOnMatchmakingCompleteDelegate_Handle(OnMatchmakingCompleteDelegate);

	// Perform matchmaking with all local players
	TArray<TSharedRef<const FUniqueNetId>> LocalPlayerIds;
	for (int32 i = 0; i < GameInstance->GetNumLocalPlayers(); ++i)
	{
		FUniqueNetIdRepl PlayerId = GameInstance->GetLocalPlayerByIndex(i)->GetPreferredUniqueNetId();
		if (PlayerId.IsValid())
		{
			LocalPlayerIds.Add((*PlayerId).AsShared());
		}
	}

	if (!Sessions->StartMatchmaking(LocalPlayerIds, NAME_GameSession, SessionSettings, QuickMatchSearchSettingsRef))
	{
		OnMatchmakingComplete(NAME_GameSession, false);
	}
}


void FSoldierMainMenu::OnSplitScreenSelectedHostOnlineLoginRequired()
{
	IOnlineIdentityPtr Identity = Online::GetIdentityInterface(GetTickableGameObjectWorld());
	if (Identity.IsValid())
	{
		int32 ControllerId = GetPlayerOwner()->GetControllerId();

		if (bIsLanMatch)
		{
			Identity->Logout(ControllerId);
			OnSplitScreenSelected();
		}
		else
		{
			OnLoginCompleteDelegateHandle = Identity->AddOnLoginCompleteDelegate_Handle(ControllerId, FOnLoginCompleteDelegate::CreateRaw(this, &FSoldierMainMenu::OnLoginCompleteHostOnline));
			Identity->Login(ControllerId, FOnlineAccountCredentials());
		}
	}
}

void FSoldierMainMenu::OnSplitScreenSelected()
{
	if (!IsMapReady())
	{
		return;
	}

	RemoveMenuFromGameViewport();

#if PLATFORM_PS4 || MAX_LOCAL_PLAYERS == 1
	if (GameInstance.IsValid() && GameInstance->GetOnlineMode() == EOnlineMode::Online)
	{
		OnUIHostTeamDeathMatch();
	}
	else
#endif
	{
		UGameViewportClient* const GVC = GEngine->GameViewport;
		GVC->AddViewportWidgetContent(SplitScreenLobbyWidgetContainer.ToSharedRef());

		SplitScreenLobbyWidget->Clear();
		FSlateApplication::Get().SetKeyboardFocus(SplitScreenLobbyWidget);
	}
}

void FSoldierMainMenu::OnHostOnlineSelected()
{
#if SHOOTER_CONSOLE_UI
	if (!ValidatePlayerIsSignedIn(GetPlayerOwner()))
	{
		return;
	}
#endif

	MatchType = EMatchType::Custom;

	EOnlineMode NewOnlineMode = bIsLanMatch ? EOnlineMode::LAN : EOnlineMode::Online;
	if (GameInstance.IsValid())
	{
		GameInstance->SetOnlineMode(NewOnlineMode);
	}
	SplitScreenLobbyWidget->SetIsJoining(false);
	MenuWidget->EnterSubMenu();
}

void FSoldierMainMenu::OnUserCanPlayHostOnline(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults)
{
	CleanupOnlinePrivilegeTask();
	MenuWidget->LockControls(false);
	if (PrivilegeResults == (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures)	
	{
		OnSplitScreenSelected();
	}
	else if (GameInstance.IsValid())
	{
		GameInstance->DisplayOnlinePrivilegeFailureDialogs(UserId, Privilege, PrivilegeResults);
	}
}

void FSoldierMainMenu::OnLoginCompleteHostOnline(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	Online::GetIdentityInterface(GetTickableGameObjectWorld())->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, OnLoginCompleteDelegateHandle);

	OnSplitScreenSelectedHostOnline();
}

void FSoldierMainMenu::OnSplitScreenSelectedHostOnline()
{
#if SHOOTER_CONSOLE_UI
	if (!ValidatePlayerForOnlinePlay(GetPlayerOwner()))
	{
		return;
	}
#endif

	StartOnlinePrivilegeTask(IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateSP(this, &FSoldierMainMenu::OnUserCanPlayHostOnline));
}
void FSoldierMainMenu::StartOnlinePrivilegeTask(const IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate& Delegate)
{
	if (GameInstance.IsValid())
	{
		// Lock controls for the duration of the async task
		MenuWidget->LockControls(true);
		FUniqueNetIdRepl UserId;
		if (PlayerOwner.IsValid())
		{
			UserId = PlayerOwner->GetPreferredUniqueNetId();
		}
		GameInstance->StartOnlinePrivilegeTask(Delegate, EUserPrivileges::CanPlayOnline, UserId.GetUniqueNetId());
	}	
}

void FSoldierMainMenu::CleanupOnlinePrivilegeTask()
{
	if (GameInstance.IsValid())
	{
		GameInstance->CleanupOnlinePrivilegeTask();
	}
}

void FSoldierMainMenu::OnHostOfflineSelected()
{
	MatchType = EMatchType::Custom;

#if LOGIN_REQUIRED_FOR_ONLINE_PLAY
	Online::GetIdentityInterface(GetTickableGameObjectWorld())->Logout(GetPlayerOwner()->GetControllerId());
#endif

	if (GameInstance.IsValid())
	{
		GameInstance->SetOnlineMode(EOnlineMode::Offline);
	}
	SplitScreenLobbyWidget->SetIsJoining( false );

	MenuWidget->EnterSubMenu();
}

FReply FSoldierMainMenu::OnSplitScreenBackedOut()
{	
	SplitScreenLobbyWidget->Clear();
	SplitScreenBackedOut();
	return FReply::Handled();
}

FReply FSoldierMainMenu::OnSplitScreenPlay()
{
	switch ( MatchType )
	{
		case EMatchType::Custom:
		{
#if SHOOTER_CONSOLE_UI
			if ( SplitScreenLobbyWidget->GetIsJoining() )
			{
#if 1
				// Until we can make split-screen menu support sub-menus, we need to do it this way
				if (GEngine && GEngine->GameViewport)
				{
					GEngine->GameViewport->RemoveViewportWidgetContent(SplitScreenLobbyWidgetContainer.ToSharedRef());
				}
				AddMenuToGameViewport();

				FSlateApplication::Get().SetKeyboardFocus(MenuWidget);	

				// Grab the map filter if there is one
				FString SelectedMapFilterName = TEXT("ANY");
				if (JoinMapOption.IsValid())
				{
					int32 FilterChoice = JoinMapOption->SelectedMultiChoice;
					if (FilterChoice != INDEX_NONE)
					{
						SelectedMapFilterName = JoinMapOption->MultiChoice[FilterChoice].ToString();
					}
				}


				MenuWidget->NextMenu = JoinServerItem->SubMenu;
				ServerListWidget->BeginServerSearch(bIsLanMatch, bIsDedicatedServer, SelectedMapFilterName);
				ServerListWidget->UpdateServerList();
				MenuWidget->EnterSubMenu();
#else
				SplitScreenLobbyWidget->NextMenu = JoinServerItem->SubMenu;
				ServerListWidget->BeginServerSearch(bIsLanMatch, bIsDedicatedServer, SelectedMapFilterName);
				ServerListWidget->UpdateServerList();
				SplitScreenLobbyWidget->EnterSubMenu();
#endif
			}
			else
#endif
			{
				if (GEngine && GEngine->GameViewport)
				{
					GEngine->GameViewport->RemoveViewportWidgetContent(SplitScreenLobbyWidgetContainer.ToSharedRef());
				}
				OnUIHostTeamDeathMatch();
			}
			break;
		}

		case EMatchType::Quick:
		{
			if (GEngine && GEngine->GameViewport)
			{
				GEngine->GameViewport->RemoveViewportWidgetContent(SplitScreenLobbyWidgetContainer.ToSharedRef());
			}
			BeginQuickMatchSearch();
			break;
		}
	}

	return FReply::Handled();
}

void FSoldierMainMenu::SplitScreenBackedOut()
{
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(SplitScreenLobbyWidgetContainer.ToSharedRef());	
	}
	AddMenuToGameViewport();

	FSlateApplication::Get().SetKeyboardFocus(MenuWidget);	
}

void FSoldierMainMenu::HelperQuickMatchSearchingUICancel(bool bShouldRemoveSession)
{
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(GetTickableGameObjectWorld());
	if (bShouldRemoveSession && Sessions.IsValid())
	{
		if (PlayerOwner.IsValid() && PlayerOwner->GetPreferredUniqueNetId().IsValid())
		{
			UGameViewportClient* const GVC = GEngine->GameViewport;
			GVC->RemoveViewportWidgetContent(QuickMatchSearchingWidgetContainer.ToSharedRef());
			GVC->AddViewportWidgetContent(QuickMatchStoppingWidgetContainer.ToSharedRef());
			FSlateApplication::Get().SetKeyboardFocus(QuickMatchStoppingWidgetContainer);
			
			OnCancelMatchmakingCompleteDelegateHandle = Sessions->AddOnCancelMatchmakingCompleteDelegate_Handle(OnCancelMatchmakingCompleteDelegate);
			Sessions->CancelMatchmaking(*PlayerOwner->GetPreferredUniqueNetId(), NAME_GameSession);
		}
	}
	else
	{
		UGameViewportClient* const GVC = GEngine->GameViewport;
		GVC->RemoveViewportWidgetContent(QuickMatchSearchingWidgetContainer.ToSharedRef());
		AddMenuToGameViewport();
		FSlateApplication::Get().SetKeyboardFocus(MenuWidget);
	}
}

FReply FSoldierMainMenu::OnQuickMatchSearchingUICancel()
{
	HelperQuickMatchSearchingUICancel(true);
	bUsedInputToCancelQuickmatchSearch = true;
	bQuickmatchSearchRequestCanceled = true;
	return FReply::Handled();
}

FReply FSoldierMainMenu::OnQuickMatchFailureUICancel()
{
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(QuickMatchFailureWidgetContainer.ToSharedRef());
	}
	AddMenuToGameViewport();
	FSlateApplication::Get().SetKeyboardFocus(MenuWidget);
	return FReply::Handled();
}

void FSoldierMainMenu::DisplayQuickmatchFailureUI()
{
	UGameViewportClient* const GVC = GEngine->GameViewport;
	RemoveMenuFromGameViewport();
	GVC->AddViewportWidgetContent(QuickMatchFailureWidgetContainer.ToSharedRef());
	FSlateApplication::Get().SetKeyboardFocus(QuickMatchFailureWidget);
}

void FSoldierMainMenu::DisplayQuickmatchSearchingUI()
{
	UGameViewportClient* const GVC = GEngine->GameViewport;
	RemoveMenuFromGameViewport();
	GVC->AddViewportWidgetContent(QuickMatchSearchingWidgetContainer.ToSharedRef());
	FSlateApplication::Get().SetKeyboardFocus(QuickMatchSearchingWidget);
	bAnimateQuickmatchSearchingUI = true;
}

void FSoldierMainMenu::OnMatchmakingComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr SessionInterface = Online::GetSessionInterface(GetTickableGameObjectWorld());
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogOnline, Warning, TEXT("OnMatchmakingComplete: Couldn't find session interface."));
		return;
	}

	SessionInterface->ClearOnMatchmakingCompleteDelegate_Handle(OnMatchmakingCompleteDelegateHandle);

	if (bQuickmatchSearchRequestCanceled && bUsedInputToCancelQuickmatchSearch)
	{
		bQuickmatchSearchRequestCanceled = false;
		// Clean up the session in case we get this event after canceling
		if (bWasSuccessful)
		{
			if (PlayerOwner.IsValid() && PlayerOwner->GetPreferredUniqueNetId().IsValid())
			{
				SessionInterface->DestroySession(NAME_GameSession);
			}
		}
		return;
	}

	if (bAnimateQuickmatchSearchingUI)
	{
		bAnimateQuickmatchSearchingUI = false;
		HelperQuickMatchSearchingUICancel(false);
		bUsedInputToCancelQuickmatchSearch = false;
	}
	else
	{
		return;
	}

	if (!bWasSuccessful)
	{
		UE_LOG(LogOnline, Warning, TEXT("Matchmaking was unsuccessful."));
		DisplayQuickmatchFailureUI();
		return;
	}

	UE_LOG(LogOnline, Log, TEXT("Matchmaking successful! Session name is %s."), *SessionName.ToString());

	if (GetPlayerOwner() == NULL)
	{
		UE_LOG(LogOnline, Warning, TEXT("OnMatchmakingComplete: No owner."));
		return;
	}

	FNamedOnlineSession* MatchmadeSession = SessionInterface->GetNamedSession(SessionName);

	if (!MatchmadeSession)
	{
		UE_LOG(LogOnline, Warning, TEXT("OnMatchmakingComplete: No session."));
		return;
	}

	if(!MatchmadeSession->OwningUserId.IsValid())
	{
		UE_LOG(LogOnline, Warning, TEXT("OnMatchmakingComplete: No session owner/host."));
		return;
	}

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(QuickMatchSearchingWidgetContainer.ToSharedRef());
	}
	bAnimateQuickmatchSearchingUI = false;

	UE_LOG(LogOnline, Log, TEXT("OnMatchmakingComplete: Session host is %d."), *MatchmadeSession->OwningUserId->ToString());

	if (ensure(GameInstance.IsValid()))
	{
		MenuWidget->LockControls(true);

		IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetTickableGameObjectWorld());
		if (Subsystem != nullptr && Subsystem->IsLocalPlayer(*MatchmadeSession->OwningUserId))
		{
			// This console is the host, start the map.
			GameInstance->BeginHostingQuickMatch();
		}
		else
		{
			// We are the client, join the host.
			GameInstance->TravelToSession(SessionName);
		}
	}
}

FSoldierMainMenu::EMap FSoldierMainMenu::GetSelectedMap() const
{
	if (GameInstance.IsValid() && GameInstance->GetOnlineMode() != EOnlineMode::Offline && HostOnlineMapOption.IsValid())
	{
		return (EMap)HostOnlineMapOption->SelectedMultiChoice;
	}
	else if (HostOfflineMapOption.IsValid())
	{
		return (EMap)HostOfflineMapOption->SelectedMultiChoice;
	}

	return EMap::ESancturary;	// Need to return something (we can hit this path in cooking)
}

void FSoldierMainMenu::CloseSubMenu()
{
	MenuWidget->MenuGoBack(true);
}

void FSoldierMainMenu::OnMenuGoBack(MenuPtr Menu)
{
	// if we are going back from options menu
	if (SoldierOptions->OptionsItem->SubMenu == Menu)
	{
		SoldierOptions->RevertChanges();
	}

	// In case a Play Together event was received, don't act on it
	// if the player changes their mind.
	if (HostOnlineMenuItem.IsValid() && HostOnlineMenuItem->SubMenu == Menu)
	{
		GameInstance->ResetPlayTogetherInfo();
	}

	// if we've backed all the way out we need to make sure online is false.
	if (MenuWidget->GetMenuLevel() == 1)
	{
		GameInstance->SetOnlineMode(EOnlineMode::Offline);
	}
}

void FSoldierMainMenu::BotCountOptionChanged(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex)
{
	BotsCountOpt = MultiOptionIndex;

	if(GetPersistentUser())
	{
		GetPersistentUser()->SetBotsCount(BotsCountOpt);
	}
}

void FSoldierMainMenu::LanMatchChanged(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex)
{
	if (HostLANItem.IsValid())
	{
		HostLANItem->SelectedMultiChoice = MultiOptionIndex;
	}

	check(JoinLANItem.IsValid());
	JoinLANItem->SelectedMultiChoice = MultiOptionIndex;
	bIsLanMatch = MultiOptionIndex > 0;
	USoldierGameUserSettings* UserSettings = CastChecked<USoldierGameUserSettings>(GEngine->GetGameUserSettings());
	UserSettings->SetLanMatch(bIsLanMatch);

	EOnlineMode NewOnlineMode = bIsLanMatch ? EOnlineMode::LAN : EOnlineMode::Online;
	if (GameInstance.IsValid())
	{
		GameInstance->SetOnlineMode(NewOnlineMode);
	}
}

void FSoldierMainMenu::DedicatedServerChanged(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex)
{
	check(DedicatedItem.IsValid());
	DedicatedItem->SelectedMultiChoice = MultiOptionIndex;
	bIsDedicatedServer = MultiOptionIndex > 0;
	USoldierGameUserSettings* UserSettings = CastChecked<USoldierGameUserSettings>(GEngine->GetGameUserSettings());
	UserSettings->SetDedicatedServer(bIsDedicatedServer);
}

void FSoldierMainMenu::RecordDemoChanged(TSharedPtr<FSoldierMenuItem> MenuItem, int32 MultiOptionIndex)
{
	if (RecordDemoItem.IsValid())
	{
		RecordDemoItem->SelectedMultiChoice = MultiOptionIndex;
	}

	bIsRecordingDemo = MultiOptionIndex > 0;

	if(GetPersistentUser())
	{
		GetPersistentUser()->SetIsRecordingDemos(bIsRecordingDemo);
		GetPersistentUser()->SaveIfDirty();
	}
}

void FSoldierMainMenu::OnUIHostFreeForAll()
{
#if WITH_EDITOR
	if (GIsEditor == true)
	{
		return;
	}
#endif
	if (!IsMapReady())
	{
		return;
	}

#if !SHOOTER_CONSOLE_UI
	if (GameInstance.IsValid())
	{
		GameInstance->SetOnlineMode(bIsLanMatch ? EOnlineMode::LAN : EOnlineMode::Online);
	}
#endif

	MenuWidget->LockControls(true);
	MenuWidget->HideMenu();

	UWorld* World = GetTickableGameObjectWorld();
	const int32 ControllerId = GetPlayerOwnerControllerId();

	if (World && ControllerId != -1)
	{
		const FSoldierMenuSoundsStyle& MenuSounds = FSoldierStyle::Get().GetWidgetStyle<FSoldierMenuSoundsStyle>("DefaultSoldierMenuSoundsStyle");
		MenuHelper::PlaySoundAndCall(World, MenuSounds.StartGameSound, ControllerId, this, &FSoldierMainMenu::HostFreeForAll);
	}
}

void FSoldierMainMenu::OnUIHostTeamDeathMatch()
{
#if WITH_EDITOR
	if (GIsEditor == true)
	{
		return;
	}
#endif
	if (!IsMapReady())
	{
		return;
	}

#if !SHOOTER_CONSOLE_UI
	if (GameInstance.IsValid())
	{
		GameInstance->SetOnlineMode(bIsLanMatch ? EOnlineMode::LAN : EOnlineMode::Online);
	}
#endif

	MenuWidget->LockControls(true);
	MenuWidget->HideMenu();

	if (GetTickableGameObjectWorld() && GetPlayerOwnerControllerId() != -1)
	{
		const FSoldierMenuSoundsStyle& MenuSounds = FSoldierStyle::Get().GetWidgetStyle<FSoldierMenuSoundsStyle>("DefaultSoldierMenuSoundsStyle");
			MenuHelper::PlaySoundAndCall(GetTickableGameObjectWorld(), MenuSounds.StartGameSound, GetPlayerOwnerControllerId(), this, &FSoldierMainMenu::HostTeamDeathMatch);
	}
}

void FSoldierMainMenu::HostGame(const FString& GameType)
{	
	if (ensure(GameInstance.IsValid()) && GetPlayerOwner() != NULL)
	{
		FString const StartURL = FString::Printf(TEXT("/Game/Maps/%s?game=%s%s%s?%s=%d%s"), *GetMapName(), *GameType, GameInstance->GetOnlineMode() != EOnlineMode::Offline ? TEXT("?listen") : TEXT(""), GameInstance->GetOnlineMode() == EOnlineMode::LAN ? TEXT("?bIsLanMatch") : TEXT(""), *ASoldierGameMode::GetBotsCountOptionName(), BotsCountOpt, bIsRecordingDemo ? TEXT("?DemoRec") : TEXT("") );

		// Game instance will handle success, failure and dialogs
		GameInstance->HostGame(GetPlayerOwner(), GameType, StartURL);
	}
}

void FSoldierMainMenu::HostFreeForAll()
{
	HostGame(TEXT("FFA"));
}

void FSoldierMainMenu::HostTeamDeathMatch()
{
	HostGame(TEXT("TDM"));
}

FReply FSoldierMainMenu::OnConfirm()
{
	if (GEngine && GEngine->GameViewport)
	{
		USoldierGameViewportClient * SoldierViewport = Cast<USoldierGameViewportClient>(GEngine->GameViewport);

		if (SoldierViewport)
		{
			// Hide the previous dialog
			SoldierViewport->HideDialog();
		}
	}

	return FReply::Handled();
}

bool FSoldierMainMenu::ValidatePlayerForOnlinePlay(ULocalPlayer* LocalPlayer)
{
	if (!ensure(GameInstance.IsValid()))
	{
		return false;
	}

	return GameInstance->ValidatePlayerForOnlinePlay(LocalPlayer);
}

bool FSoldierMainMenu::ValidatePlayerIsSignedIn(ULocalPlayer* LocalPlayer)
{
	if (!ensure(GameInstance.IsValid()))
	{
		return false;
	}

	return GameInstance->ValidatePlayerIsSignedIn(LocalPlayer);
}

void FSoldierMainMenu::OnJoinServerLoginRequired()
{
	IOnlineIdentityPtr Identity = Online::GetIdentityInterface(GetTickableGameObjectWorld());
	if (Identity.IsValid())
	{
		int32 ControllerId = GetPlayerOwner()->GetControllerId();
	
		if (bIsLanMatch)
		{
			Identity->Logout(ControllerId);
			OnUserCanPlayOnlineJoin(*GetPlayerOwner()->GetCachedUniqueNetId(), EUserPrivileges::CanPlayOnline, (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures);
		}
		else
		{
			OnLoginCompleteDelegateHandle = Identity->AddOnLoginCompleteDelegate_Handle(ControllerId, FOnLoginCompleteDelegate::CreateRaw(this, &FSoldierMainMenu::OnLoginCompleteJoin));
			Identity->Login(ControllerId, FOnlineAccountCredentials());
		}
	}
}

void FSoldierMainMenu::OnLoginCompleteJoin(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	IOnlineIdentityPtr Identity = Online::GetIdentityInterface(GetTickableGameObjectWorld());
	if (Identity.IsValid())
	{
		Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, OnLoginCompleteDelegateHandle);
	}

	OnJoinServer();
}

void FSoldierMainMenu::OnJoinSelected()
{
#if SHOOTER_CONSOLE_UI
	if (!ValidatePlayerIsSignedIn(GetPlayerOwner()))
	{
		return;
	}
#endif

	MenuWidget->EnterSubMenu();
}

void FSoldierMainMenu::OnJoinServer()
{
#if SHOOTER_CONSOLE_UI
	if ( !ValidatePlayerForOnlinePlay(GetPlayerOwner()) )
	{
		return;
	}
#endif

	StartOnlinePrivilegeTask(IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateSP(this, &FSoldierMainMenu::OnUserCanPlayOnlineJoin));
}

void FSoldierMainMenu::OnUserCanPlayOnlineJoin(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults)
{
	CleanupOnlinePrivilegeTask();
	MenuWidget->LockControls(false);

	if (PrivilegeResults == (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures)
	{

		//make sure to switch to custom match type so we don't instead use Quick type
		MatchType = EMatchType::Custom;

		if (GameInstance.IsValid())
		{
			GameInstance->SetOnlineMode(bIsLanMatch ? EOnlineMode::LAN : EOnlineMode::Online);
		}

		MatchType = EMatchType::Custom;
		// Grab the map filter if there is one
		FString SelectedMapFilterName("Any");
		if( JoinMapOption.IsValid())
		{
			int32 FilterChoice = JoinMapOption->SelectedMultiChoice;
			if( FilterChoice != INDEX_NONE )
			{
				SelectedMapFilterName = JoinMapOption->MultiChoice[FilterChoice].ToString();
			}
		}

#if SHOOTER_CONSOLE_UI
		UGameViewportClient* const GVC = GEngine->GameViewport;
#if PLATFORM_PS4 || MAX_LOCAL_PLAYERS == 1
		// Show server menu (skip splitscreen)
		AddMenuToGameViewport();
		FSlateApplication::Get().SetKeyboardFocus(MenuWidget);

		MenuWidget->NextMenu = JoinServerItem->SubMenu;
		ServerListWidget->BeginServerSearch(bIsLanMatch, bIsDedicatedServer, SelectedMapFilterName);
		ServerListWidget->UpdateServerList();
		MenuWidget->EnterSubMenu();
#else
		// Show splitscreen menu
		RemoveMenuFromGameViewport();	
		GVC->AddViewportWidgetContent(SplitScreenLobbyWidgetContainer.ToSharedRef());

		SplitScreenLobbyWidget->Clear();
		FSlateApplication::Get().SetKeyboardFocus(SplitScreenLobbyWidget);

		SplitScreenLobbyWidget->SetIsJoining( true );
#endif
#else
		MenuWidget->NextMenu = JoinServerItem->SubMenu;
		//FString SelectedMapFilterName = JoinMapOption->MultiChoice[JoinMapOption->SelectedMultiChoice].ToString();

		ServerListWidget->BeginServerSearch(bIsLanMatch, bIsDedicatedServer, SelectedMapFilterName);
		ServerListWidget->UpdateServerList();
		MenuWidget->EnterSubMenu();
#endif
	}
	else if (GameInstance.IsValid())
	{
		GameInstance->DisplayOnlinePrivilegeFailureDialogs(UserId, Privilege, PrivilegeResults);
	}
}

void FSoldierMainMenu::OnShowLeaderboard()
{
	MenuWidget->NextMenu = LeaderboardItem->SubMenu;
#if LOGIN_REQUIRED_FOR_ONLINE_PLAY
	LeaderboardWidget->ReadStatsLoginRequired();
#else
	LeaderboardWidget->ReadStats();
#endif
	MenuWidget->EnterSubMenu();
}

void FSoldierMainMenu::OnShowOnlineStore()
{
	MenuWidget->NextMenu = OnlineStoreItem->SubMenu;
#if LOGIN_REQUIRED_FOR_ONLINE_PLAY
	UE_LOG(LogOnline, Warning, TEXT("You need to be logged in before using the store"));
#endif
	OnlineStoreWidget->BeginGettingOffers();
	MenuWidget->EnterSubMenu();
}

void FSoldierMainMenu::OnShowDemoBrowser()
{
	MenuWidget->NextMenu = DemoBrowserItem->SubMenu;
	DemoListWidget->BuildDemoList();
	MenuWidget->EnterSubMenu();
}

void FSoldierMainMenu::OnUIQuit()
{
	LockAndHideMenu();

	const FSoldierMenuSoundsStyle& MenuSounds = FSoldierStyle::Get().GetWidgetStyle<FSoldierMenuSoundsStyle>("DefaultSoldierMenuSoundsStyle");

	if (GetTickableGameObjectWorld() != NULL && GetPlayerOwnerControllerId() != -1)
	{
		FSlateApplication::Get().PlaySound(MenuSounds.ExitGameSound, GetPlayerOwnerControllerId());
		MenuHelper::PlaySoundAndCall(GetTickableGameObjectWorld(), MenuSounds.ExitGameSound, GetPlayerOwnerControllerId(), this, &FSoldierMainMenu::Quit);
	}
}

void FSoldierMainMenu::Quit()
{
	if (ensure(GameInstance.IsValid()))
	{
		UGameViewportClient* const Viewport = GameInstance->GetGameViewportClient();
		if (ensure(Viewport)) 
		{
			Viewport->ConsoleCommand("quit");
		}
	}
}

void FSoldierMainMenu::LockAndHideMenu()
{
	MenuWidget->LockControls(true);
	MenuWidget->HideMenu();
}

void FSoldierMainMenu::DisplayLoadingScreen()
{
	ISoldierGameLoadingScreenModule* LoadingScreenModule = FModuleManager::LoadModulePtr<ISoldierGameLoadingScreenModule>("SoldierGameLoadingScreen");
	if( LoadingScreenModule != NULL )
	{
		LoadingScreenModule->StartInGameLoadingScreen();
	}
}

bool FSoldierMainMenu::IsMapReady() const
{
	bool bReady = true;
	IPlatformChunkInstall* ChunkInstaller = FPlatformMisc::GetPlatformChunkInstall();
	if (ChunkInstaller)
	{
		EMap SelectedMap = GetSelectedMap();
		// should use the AssetRegistry as soon as maps are added to the AssetRegistry
		int32 MapChunk = ChunkMapping[(int)SelectedMap];
		EChunkLocation::Type ChunkLocation = ChunkInstaller->GetChunkLocation(MapChunk);
		if (ChunkLocation == EChunkLocation::NotAvailable)
		{			
			bReady = false;
		}
	}
	return bReady;
}

USoldierPersistentUser* FSoldierMainMenu::GetPersistentUser() const
{
	USoldierLocalPlayer* const SoldierLocalPlayer = Cast<USoldierLocalPlayer>(GetPlayerOwner());
	return SoldierLocalPlayer ? SoldierLocalPlayer->GetPersistentUser() : nullptr;
}

UWorld* FSoldierMainMenu::GetTickableGameObjectWorld() const
{
	ULocalPlayer* LocalPlayerOwner = GetPlayerOwner();
	return (LocalPlayerOwner ? LocalPlayerOwner->GetWorld() : nullptr);
}

ULocalPlayer* FSoldierMainMenu::GetPlayerOwner() const
{
	return PlayerOwner.Get();
}

int32 FSoldierMainMenu::GetPlayerOwnerControllerId() const
{
	return ( PlayerOwner.IsValid() ) ? PlayerOwner->GetControllerId() : -1;
}

FString FSoldierMainMenu::GetMapName() const
{
	return MapNames[(int)GetSelectedMap()];
}

void FSoldierMainMenu::OnCancelMatchmakingComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(GetTickableGameObjectWorld());
	if (Sessions.IsValid())
	{
		Sessions->ClearOnCancelMatchmakingCompleteDelegate_Handle(OnCancelMatchmakingCompleteDelegateHandle);
	}

	bAnimateQuickmatchSearchingUI = false;
	UGameViewportClient* const GVC = GEngine->GameViewport;
	GVC->RemoveViewportWidgetContent(QuickMatchStoppingWidgetContainer.ToSharedRef());
	AddMenuToGameViewport();
	FSlateApplication::Get().SetKeyboardFocus(MenuWidget);
}

void FSoldierMainMenu::OnPlayTogetherEventReceived()
{
	HostOnlineMenuItem->Widget->SetMenuItemActive(true);
	MenuWidget->ConfirmMenuItem();
}

#undef LOCTEXT_NAMESPACE

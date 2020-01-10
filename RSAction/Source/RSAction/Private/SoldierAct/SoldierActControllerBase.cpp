// Copyright 1998-2019 Epic Games, Inc.All Rights Reserved.
#include "SoldierActControllerBase.h"
#include "RSAction.h"
#include "SoldierGameSession.h"
#include "Online/SoldierOnlineGameSettings.h"
#include "OnlineSubsystemSessionSettings.h"
#include "OnlineSubsystemUtils.h"

void USoldierActControllerBase::OnInit()
{
	bIsLoggedIn              = false;
	bIsLoggingIn             = false;
	bInQuickMatchSearch      = false;
	bFoundQuickMatchGame     = false;
	bIsSearchingForGame      = false;
	bFoundGame               = false;
	NumOfCycledMatches       = 0;

	if (!FParse::Value(FCommandLine::Get(), TEXT("TargetNumOfCycledMatches"), TargetNumOfCycledMatches))
	{
		TargetNumOfCycledMatches = 2;
	}
}

void USoldierActControllerBase::OnTick(float TimeDelta)
{
	const FName GameInstanceState = GetGameInstanceState();

	if (GameInstanceState == SoldierGameInstanceState::WelcomeScreen)
	{
		if (!bIsLoggedIn && !bIsLoggingIn)
		{
			bIsLoggingIn = true;
			StartPlayerLoginProcess();
		}
	}
	else if (GameInstanceState == SoldierGameInstanceState::MainMenu)
	{
		if (!bIsLoggedIn && !bIsLoggingIn)
		{
			ULocalPlayer* LP = GetFirstLocalPlayer();

			if (LP)
			{
				bIsLoggingIn = true;
				TSharedPtr<const FUniqueNetId> UserId = LP->GetPreferredUniqueNetId().GetUniqueNetId();
				OnUserCanPlay(*UserId, EUserPrivileges::CanPlay, (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures);
			}
		}
	}
	else if (GameInstanceState == SoldierGameInstanceState::MessageMenu)
	{
		UE_LOG(LogGauntlet, Error, TEXT("Failing due to MessageMenu!"));
		EndTest(-1);
		return;
	}
}

void USoldierActControllerBase::OnPostMapChange(UWorld* World)
{
	if (IsInGame())
	{
		if (++NumOfCycledMatches >= TargetNumOfCycledMatches)
		{
			EndTest(0);
		}
	}
	else if (NumOfCycledMatches > 0)
	{
		UE_LOG(LogGauntlet, Error, TEXT("Failed to cycle match TargetNumOfCycledMatches(%i)!  NumOfCycledMatches = %i"), TargetNumOfCycledMatches, NumOfCycledMatches);
		EndTest(-1);
	}
}

void USoldierActControllerBase::StartPlayerLoginProcess()
{
	const IOnlineIdentityPtr IdentityInterface = Online::GetIdentityInterface(GetWorld());

	if (IdentityInterface.IsValid())
	{
		TSharedPtr<GenericApplication> GenericApplication = FSlateApplication::Get().GetPlatformApplication();
		const bool bIsLicensed = GenericApplication->ApplicationLicenseValid();

		const ELoginStatus::Type LoginStatus = IdentityInterface->GetLoginStatus(0);
		if (LoginStatus == ELoginStatus::NotLoggedIn || !bIsLicensed)
		{
			UE_LOG(LogGauntlet, Error, TEXT("Failed!  No player is logged in!"));
			EndTest(-1);
			return;
		}

		TSharedPtr<const FUniqueNetId> UserId = IdentityInterface->GetUniquePlayerId(0);

		if (UserId.IsValid())
		{
			IdentityInterface->GetUserPrivilege(*UserId, EUserPrivileges::CanPlay, IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateSP(this, &USoldierActControllerBase::OnUserCanPlay));
		}
		else
		{
			UE_LOG(LogGauntlet, Error, TEXT("Failed!  Player has invalid UniqueNetId!"));
			EndTest(-1);
		}
	}
}

void USoldierActControllerBase::OnUserCanPlay(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults)
{
	if (PrivilegeResults == (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures)
	{
		ULocalPlayer* NewPlayerOwner = GetFirstLocalPlayer();

		if (NewPlayerOwner)
		{
			NewPlayerOwner->SetControllerId(0);
			NewPlayerOwner->SetCachedUniqueNetId(NewPlayerOwner->GetUniqueNetIdFromCachedControllerId().GetUniqueNetId());
		}
		else
		{
			UE_LOG(LogGauntlet, Error, TEXT("Failed!  Could not find LocalPlayer in OnUserCanPlay!"));
			EndTest(-1);
		}

#if SHOOTER_CONSOLE_UI
#if LOGIN_REQUIRED_FOR_ONLINE_PLAY
		StartLoginTask();
#else
		StartOnlinePrivilegeTask();
#endif //LOGIN_REQUIRED_FOR_ONLINE_PLAY
#else
		OnUserCanPlayOnline(*NewPlayerOwner->GetCachedUniqueNetId().GetUniqueNetId(), EUserPrivileges::CanPlayOnline, (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures);
#endif //SHOOTER_CONSOLE_UI
	}
	else
	{
		UE_LOG(LogGauntlet, Error, TEXT("Failed!  Player does not have appropiate privileges to play!"));
		EndTest(-1);
	}
}

void USoldierActControllerBase::StartLoginTask()
{
	const IOnlineIdentityPtr IdentityInterface = Online::GetIdentityInterface(GetWorld());

	if (IdentityInterface.IsValid())
	{
		OnLoginCompleteDelegateHandle = IdentityInterface->AddOnLoginCompleteDelegate_Handle(0, FOnLoginCompleteDelegate::CreateSP(this, &USoldierActControllerBase::OnLoginTaskComplete));
		IdentityInterface->Login(0, FOnlineAccountCredentials());
	}
	else
	{
		UE_LOG(LogGauntlet, Error, TEXT("Failed!  IdentityInterface is not valid in StartLoginTask!"));
		EndTest(-1);
	}
}

void USoldierActControllerBase::OnLoginTaskComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	const IOnlineIdentityPtr IdentityInterface = Online::GetIdentityInterface(GetWorld());

	if (IdentityInterface.IsValid())
	{
		IdentityInterface->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, OnLoginCompleteDelegateHandle);

		if (bWasSuccessful)
		{
			StartOnlinePrivilegeTask();
		}
		else
		{
			UE_LOG(LogGauntlet, Error, TEXT("Failed!  Player failed to login!"));
			EndTest(-1);
		}
	}
	else
	{
		UE_LOG(LogGauntlet, Error, TEXT("Failed!  IdentityInterface is not valid in OnLoginTaskComplete!"));
		EndTest(-1);
	}


}

void USoldierActControllerBase::StartOnlinePrivilegeTask()
{
	const ULocalPlayer* PlayerOwner            = GetFirstLocalPlayer();
	const IOnlineIdentityPtr IdentityInterface = Online::GetIdentityInterface(GetWorld());

	if (PlayerOwner && IdentityInterface.IsValid())
	{
		IdentityInterface->GetUserPrivilege(*PlayerOwner->GetCachedUniqueNetId().GetUniqueNetId(), EUserPrivileges::CanPlayOnline,
			IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateSP(this, &USoldierActControllerBase::OnUserCanPlayOnline));
	}
	else
	{
		UE_LOG(LogGauntlet, Error, TEXT("Failed!  Could not find LocalPlayer or IdentityInterface is null in OnlinePrivilegeTask!"));
		EndTest(-1);
	}
}

void USoldierActControllerBase::OnUserCanPlayOnline(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults)
{
	if (PrivilegeResults == (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures)
	{
		bIsLoggingIn = false;
		bIsLoggedIn = true;

		if (USoldierGameInstance* GameInstance = GetGameInstance())
		{
			GameInstance->SetOnlineMode(EOnlineMode::Online);
		}
	}
	else
	{
		UE_LOG(LogGauntlet, Error, TEXT("Failed!  Player does not have appropiate privileges to play online!"));
		EndTest(-1);
	}
}

void USoldierActControllerBase::HostGame()
{
	USoldierGameInstance* GameInstance = GetGameInstance();
	ULocalPlayer* PlayerOwner          = GameInstance ? GameInstance->GetFirstGamePlayer() : nullptr;

	if (PlayerOwner)
	{
		const FString GameType = TEXT("TDM");
		const FString StartURL = FString::Printf(TEXT("/Game/Maps/%s?game=%s%s"), TEXT("Sanctuary"), *GameType, TEXT("?listen"));

		GameInstance->HostGame(PlayerOwner, GameType, StartURL);
	}
	else
	{
		UE_LOG(LogGauntlet, Error, TEXT("Failed!  Could not find LocalPlayer or GameInstance is null!"));
		EndTest(-1);
	}
}

void USoldierActControllerBase::StartQuickMatch()
{
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(GetWorld());
	USoldierGameInstance* GameInstance = GetGameInstance();
	if (!Sessions.IsValid() || GameInstance == nullptr)
	{
		UE_LOG(LogGauntlet, Error, TEXT("Failed!  Could not find online session interface or GameInstance is null!"));
		EndTest(-1);
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

	Sessions->ClearOnMatchmakingCompleteDelegate_Handle(OnMatchmakingCompleteDelegateHandle);
	OnMatchmakingCompleteDelegateHandle = Sessions->AddOnMatchmakingCompleteDelegate_Handle(FOnMatchmakingCompleteDelegate::CreateSP(this, &USoldierActControllerBase::OnMatchmakingComplete));

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

	bInQuickMatchSearch = true;

	if (!Sessions->StartMatchmaking(LocalPlayerIds, NAME_GameSession, SessionSettings, QuickMatchSearchSettingsRef))
	{
		OnMatchmakingComplete(NAME_GameSession, false);
	}
}

void USoldierActControllerBase::OnMatchmakingComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr SessionInterface = Online::GetSessionInterface(GetWorld());
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogGauntlet, Error, TEXT("Failed!  Could not find online session interface!"));
		EndTest(-1);
		return;
	}

	bInQuickMatchSearch = false;
	SessionInterface->ClearOnMatchmakingCompleteDelegate_Handle(OnMatchmakingCompleteDelegateHandle);

	if (!bWasSuccessful)
	{
		UE_LOG(LogGauntlet, Warning, TEXT("Matchmaking was unsuccessful."));
		return;
	}

	UE_LOG(LogGauntlet, Log, TEXT("Matchmaking successful! Session name is %s."), *SessionName.ToString());

	FNamedOnlineSession* MatchmadeSession = SessionInterface->GetNamedSession(SessionName);

	if (!MatchmadeSession)
	{
		UE_LOG(LogGauntlet, Warning, TEXT("OnMatchmakingComplete: No session."));
		return;
	}

	if (!MatchmadeSession->OwningUserId.IsValid())
	{
		UE_LOG(LogGauntlet, Warning, TEXT("OnMatchmakingComplete: No session owner/host."));
		return;
	}

	UE_LOG(LogGauntlet, Log, TEXT("OnMatchmakingComplete: Session host is %d."), *MatchmadeSession->OwningUserId->ToString());

	if (USoldierGameInstance* GameInstance = GetGameInstance())
	{
		IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());

		// We only care about hosted games 
		if (Subsystem && Subsystem->IsLocalPlayer(*MatchmadeSession->OwningUserId))
		{
			SessionInterface->EndSession(SessionName);
		}
		else
		{
			bFoundQuickMatchGame = true;
			GameInstance->TravelToSession(SessionName);
		}
	}
}

void USoldierActControllerBase::StartSearchingForGame()
{
	if (USoldierGameInstance* GameInstance = GetGameInstance())
	{
		bIsSearchingForGame = true;
		GameInstance->FindSessions(GameInstance->GetFirstGamePlayer(), false, false);
	}
}

void USoldierActControllerBase::UpdateSearchStatus()
{
	ASoldierGameSession* SoldierSession = GetGameSession();

	if (SoldierSession)
	{
		int32 CurrentSearchIdx, NumSearchResults;
		EOnlineAsyncTaskState::Type SearchState = SoldierSession->GetSearchResultStatus(CurrentSearchIdx, NumSearchResults);

		UE_LOG(LogGauntlet, Log, TEXT("SoldierSession->GetSearchResultStatus: %s"), EOnlineAsyncTaskState::ToString(SearchState));

		switch (SearchState)
		{
		case EOnlineAsyncTaskState::InProgress:
		{
			break;
		}
		case EOnlineAsyncTaskState::Done:
		{
			const TArray<FOnlineSessionSearchResult> & SearchResults = SoldierSession->GetSearchResults();
			check(SearchResults.Num() == NumSearchResults);

			if (NumSearchResults > 0)
			{
				bFoundGame = true;

				USoldierGameInstance* GameInstance = GetGameInstance();
				ULocalPlayer* PlayerOwner          = GameInstance ? GameInstance->GetFirstGamePlayer() : nullptr;

				if (PlayerOwner)
				{
					// Join First Result
					GameInstance->JoinSession(PlayerOwner, 0);
				}
			}

			bIsSearchingForGame = false;

			break;
		}

		case EOnlineAsyncTaskState::Failed:
		case EOnlineAsyncTaskState::NotStarted:
		default:
		{
			bIsSearchingForGame = false;
			break;
		}
		}
	}
}

USoldierGameInstance* USoldierActControllerBase::GetGameInstance() const 
{
	if (const UWorld* World = GetWorld())
	{
		return Cast<USoldierGameInstance>(GetWorld()->GetGameInstance());
	}

	return nullptr;
}

const FName USoldierActControllerBase::GetGameInstanceState() const 
{
	if (const USoldierGameInstance* GameInstance = GetGameInstance())
	{
		return GameInstance->GetCurrentState();
	}

	return "";
}

ASoldierGameSession* USoldierActControllerBase::GetGameSession() const
{
	if (const USoldierGameInstance* GameInstance = GetGameInstance())
	{
		return GameInstance->GetGameSession();
	}

	return nullptr;
}

bool USoldierActControllerBase::IsInGame() const
{
	return GetGameInstanceState() == SoldierGameInstanceState::Playing;
}

ULocalPlayer* USoldierActControllerBase::GetFirstLocalPlayer() const
{
	if (const USoldierGameInstance* GameInstance = GetGameInstance())
	{
		return GameInstance->GetFirstGamePlayer();
	}

	return nullptr;
}
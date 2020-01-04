// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SoldierGame.h"
#include "Player/SoldierPlayerController.h"
#include "Player/SoldierPlayerCameraManager.h"
#include "Player/SoldierCheatManager.h"
#include "Player/SoldierLocalPlayer.h"
#include "Online/SoldierPlayerState.h"
#include "Weapons/SoldierWeapon.h"
#include "UI/Menu/SoldierIngameMenu.h"
#include "UI/Style/SoldierStyle.h"
#include "UI/SoldierHUD.h"
#include "Online.h"
#include "..\..\Engine\Plugins\Online\OnlineSubsystem\Source\Public\Interfaces\OnlineAchievementsInterface.h"
#include "..\..\Engine\Plugins\Online\OnlineSubsystem\Source\Public\Interfaces\OnlineEventsInterface.h"
#include "..\..\Engine\Plugins\Online\OnlineSubsystem\Source\Public\Interfaces\OnlineStatsInterface.h"
#include "..\..\Engine\Plugins\Online\OnlineSubsystem\Source\Public\Interfaces\OnlineIdentityInterface.h"
#include "..\..\Engine\Plugins\Online\OnlineSubsystem\Source\Public\Interfaces\OnlineSessionInterface.h"
#include "SoldierGameInstance.h"
#include "Leaderboards.h"
#include "SoldierGameViewportClient.h"
#include "Sound/SoundNodeLocalPlayer.h"
#include "AudioThread.h"
#include "OnlineSubsystemUtils.h"

#define  ACH_FRAG_SOMEONE	TEXT("ACH_FRAG_SOMEONE")
#define  ACH_SOME_KILLS		TEXT("ACH_SOME_KILLS")
#define  ACH_LOTS_KILLS		TEXT("ACH_LOTS_KILLS")
#define  ACH_FINISH_MATCH	TEXT("ACH_FINISH_MATCH")
#define  ACH_LOTS_MATCHES	TEXT("ACH_LOTS_MATCHES")
#define  ACH_FIRST_WIN		TEXT("ACH_FIRST_WIN")
#define  ACH_LOTS_WIN		TEXT("ACH_LOTS_WIN")
#define  ACH_MANY_WIN		TEXT("ACH_MANY_WIN")
#define  ACH_SHOOT_BULLETS	TEXT("ACH_SHOOT_BULLETS")
#define  ACH_SHOOT_ROCKETS	TEXT("ACH_SHOOT_ROCKETS")
#define  ACH_GOOD_SCORE		TEXT("ACH_GOOD_SCORE")
#define  ACH_GREAT_SCORE	TEXT("ACH_GREAT_SCORE")
#define  ACH_PLAY_SANCTUARY	TEXT("ACH_PLAY_SANCTUARY")
#define  ACH_PLAY_HIGHRISE	TEXT("ACH_PLAY_HIGHRISE")

static const int32 SomeKillsCount = 10;
static const int32 LotsKillsCount = 20;
static const int32 LotsMatchesCount = 5;
static const int32 LotsWinsCount = 3;
static const int32 ManyWinsCount = 5;
static const int32 LotsBulletsCount = 100;
static const int32 LotsRocketsCount = 10;
static const int32 GoodScoreCount = 10;
static const int32 GreatScoreCount = 15;

ASoldierPlayerController::ASoldierPlayerController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PlayerCameraManagerClass = ASoldierPlayerCameraManager::StaticClass();
	CheatClass = USoldierCheatManager::StaticClass();
	bAllowGameActions = true;
	bGameEndedFrame = false;
	LastDeathLocation = FVector::ZeroVector;

	ServerSayString = TEXT("Say");
	SoldierFriendUpdateTimer = 0.0f;
	bHasSentStartEvents = false;

	StatMatchesPlayed = 0;
	StatKills = 0;
	StatDeaths = 0;
	bHasFetchedPlatformData = false;
}

void ASoldierPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// UI input
	InputComponent->BindAction("InGameMenu", IE_Pressed, this, &ASoldierPlayerController::OnToggleInGameMenu);
	InputComponent->BindAction("Scoreboard", IE_Pressed, this, &ASoldierPlayerController::OnShowScoreboard);
	InputComponent->BindAction("Scoreboard", IE_Released, this, &ASoldierPlayerController::OnHideScoreboard);
	InputComponent->BindAction("ConditionalCloseScoreboard", IE_Pressed, this, &ASoldierPlayerController::OnConditionalCloseScoreboard);
	InputComponent->BindAction("ToggleScoreboard", IE_Pressed, this, &ASoldierPlayerController::OnToggleScoreboard);

	// voice chat
	InputComponent->BindAction("PushToTalk", IE_Pressed, this, &APlayerController::StartTalking);
	InputComponent->BindAction("PushToTalk", IE_Released, this, &APlayerController::StopTalking);

	InputComponent->BindAction("ToggleChat", IE_Pressed, this, &ASoldierPlayerController::ToggleChatWindow);
}


void ASoldierPlayerController::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	FSoldierStyle::Initialize();
	SoldierFriendUpdateTimer = 0;
}

void ASoldierPlayerController::ClearLeaderboardDelegate()
{
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineLeaderboardsPtr Leaderboards = OnlineSub->GetLeaderboardsInterface();
		if (Leaderboards.IsValid())
		{
			Leaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(LeaderboardReadCompleteDelegateHandle);
		}
	}
}

void ASoldierPlayerController::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);

	if (IsGameMenuVisible())
	{
		if (SoldierFriendUpdateTimer > 0)
		{
			SoldierFriendUpdateTimer -= DeltaTime;
		}
		else
		{
			TSharedPtr<class FSoldierFriends> SoldierFriends = SoldierIngameMenu->GetSoldierFriends();
			ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
			if (SoldierFriends.IsValid() && LocalPlayer && LocalPlayer->GetControllerId() >= 0)
			{
				SoldierFriends->UpdateFriends(LocalPlayer->GetControllerId());
			}
			
			// Make sure the time between calls is long enough that we won't trigger (0x80552C81) and not exceed the web api rate limit
			// That value is currently 75 requests / 15 minutes.
			SoldierFriendUpdateTimer = 15;

		}
	}

	// Is this the first frame after the game has ended
	if(bGameEndedFrame)
	{
		bGameEndedFrame = false;

		// ONLY PUT CODE HERE WHICH YOU DON'T WANT TO BE DONE DUE TO HOST LOSS

		// Do we need to show the end of round scoreboard?
		if (IsPrimaryPlayer())
		{
			ASoldierHUD* SoldierHUD = GetSoldierHUD();
			if (SoldierHUD)
			{
				SoldierHUD->ShowScoreboard(true, true);
			}
		}
	}

	const bool bLocallyControlled = IsLocalController();
	const uint32 UniqueID = GetUniqueID();
	FAudioThread::RunCommandOnAudioThread([UniqueID, bLocallyControlled]()
	{
		USoundNodeLocalPlayer::GetLocallyControlledActorCache().Add(UniqueID, bLocallyControlled);
	});
};

void ASoldierPlayerController::BeginDestroy()
{
	Super::BeginDestroy();
	ClearLeaderboardDelegate();

	// clear any online subsystem references
	SoldierIngameMenu = nullptr;

	if (!GExitPurge)
	{
		const uint32 UniqueID = GetUniqueID();
		FAudioThread::RunCommandOnAudioThread([UniqueID]()
		{
			USoundNodeLocalPlayer::GetLocallyControlledActorCache().Remove(UniqueID);
		});
	}
}

void ASoldierPlayerController::SetPlayer( UPlayer* InPlayer )
{
	Super::SetPlayer( InPlayer );

	if (ULocalPlayer* const LocalPlayer = Cast<ULocalPlayer>(Player))
	{
		//Build menu only after game is initialized
		SoldierIngameMenu = MakeShareable(new FSoldierIngameMenu());
		SoldierIngameMenu->Construct(Cast<ULocalPlayer>(Player));

		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
	}
}

void ASoldierPlayerController::QueryAchievements()
{
	// precache achievements
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	if (LocalPlayer && LocalPlayer->GetControllerId() != -1)
	{
		IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
		if(OnlineSub)
		{
			IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
			if (Identity.IsValid())
			{
				TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(LocalPlayer->GetControllerId());

				if (UserId.IsValid())
				{
					IOnlineAchievementsPtr Achievements = OnlineSub->GetAchievementsInterface();

					if (Achievements.IsValid())
					{
						Achievements->QueryAchievements( *UserId.Get(), FOnQueryAchievementsCompleteDelegate::CreateUObject( this, &ASoldierPlayerController::OnQueryAchievementsComplete ));
					}
				}
				else
				{
					UE_LOG(LogOnline, Warning, TEXT("No valid user id for this controller."));
				}
			}
			else
			{
				UE_LOG(LogOnline, Warning, TEXT("No valid identity interface."));
			}
		}
		else
		{
			UE_LOG(LogOnline, Warning, TEXT("No default online subsystem."));
		}
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("No local player, cannot read achievements."));
	}
}

void ASoldierPlayerController::OnQueryAchievementsComplete(const FUniqueNetId& PlayerId, const bool bWasSuccessful )
{
	UE_LOG(LogOnline, Display, TEXT("ASoldierPlayerController::OnQueryAchievementsComplete(bWasSuccessful = %s)"), bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"));
}

void ASoldierPlayerController::OnLeaderboardReadComplete(bool bWasSuccessful)
{
	if (ReadObject.IsValid() && ReadObject->ReadState == EOnlineAsyncTaskState::Done && !bHasFetchedPlatformData)
	{
		bHasFetchedPlatformData = true;
		ClearLeaderboardDelegate();

		// We should only have one stat.
		if (bWasSuccessful && ReadObject->Rows.Num() == 1)
		{
			FOnlineStatsRow& RowData = ReadObject->Rows[0];
			if (const FVariantData* KillData = RowData.Columns.Find(LEADERBOARD_STAT_KILLS))
			{
				KillData->GetValue(StatKills);
			}

			if (const FVariantData* DeathData = RowData.Columns.Find(LEADERBOARD_STAT_DEATHS))
			{
				DeathData->GetValue(StatDeaths);
			}

			if (const FVariantData* MatchData = RowData.Columns.Find(LEADERBOARD_STAT_MATCHESPLAYED))
			{
				MatchData->GetValue(StatMatchesPlayed);
			}

			UE_LOG(LogOnline, Log, TEXT("Fetched player stat data. Kills %d Deaths %d Matches %d"), StatKills, StatDeaths, StatMatchesPlayed);
		}
	}
}

void ASoldierPlayerController::QueryStats()
{
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	if (LocalPlayer && LocalPlayer->GetControllerId() != -1)
	{
		IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
		if (OnlineSub)
		{
			IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
			if (Identity.IsValid())
			{
				TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(LocalPlayer->GetControllerId());

				if (UserId.IsValid())
				{
					IOnlineLeaderboardsPtr Leaderboards = OnlineSub->GetLeaderboardsInterface();
					if (Leaderboards.IsValid() && !bHasFetchedPlatformData)
					{
						TArray<TSharedRef<const FUniqueNetId>> QueryPlayers;
						QueryPlayers.Add(UserId.ToSharedRef());

						LeaderboardReadCompleteDelegateHandle = Leaderboards->OnLeaderboardReadCompleteDelegates.AddUObject(this, &ASoldierPlayerController::OnLeaderboardReadComplete);
						ReadObject = MakeShareable(new FSoldierAllTimeMatchResultsRead());
						FOnlineLeaderboardReadRef ReadObjectRef = ReadObject.ToSharedRef();
						if (Leaderboards->ReadLeaderboards(QueryPlayers, ReadObjectRef))
						{
							UE_LOG(LogOnline, Log, TEXT("Started process to fetch stats for current user."));
						}
						else
						{
							UE_LOG(LogOnline, Warning, TEXT("Could not start leaderboard fetch process. This will affect stat writes for this session."));
						}
						
					}
				}
			}
		}
	}
}

void ASoldierPlayerController::UnFreeze()
{
	ServerRestartPlayer();
}

void ASoldierPlayerController::FailedToSpawnPawn()
{
	if(StateName == NAME_Inactive)
	{
		BeginInactiveState();
	}
	Super::FailedToSpawnPawn();
}

void ASoldierPlayerController::PawnPendingDestroy(APawn* P)
{
	LastDeathLocation = P->GetActorLocation();
	FVector CameraLocation = LastDeathLocation + FVector(0, 0, 300.0f);
	FRotator CameraRotation(-90.0f, 0.0f, 0.0f);
	FindDeathCameraSpot(CameraLocation, CameraRotation);

	Super::PawnPendingDestroy(P);

	ClientSetSpectatorCamera(CameraLocation, CameraRotation);
}

void ASoldierPlayerController::GameHasEnded(class AActor* EndGameFocus, bool bIsWinner)
{
	Super::GameHasEnded(EndGameFocus, bIsWinner);
}

void ASoldierPlayerController::ClientSetSpectatorCamera_Implementation(FVector CameraLocation, FRotator CameraRotation)
{
	SetInitialLocationAndRotation(CameraLocation, CameraRotation);
	SetViewTarget(this);
}

bool ASoldierPlayerController::FindDeathCameraSpot(FVector& CameraLocation, FRotator& CameraRotation)
{
	const FVector PawnLocation = GetPawn()->GetActorLocation();
	FRotator ViewDir = GetControlRotation();
	ViewDir.Pitch = -45.0f;

	const float YawOffsets[] = { 0.0f, -180.0f, 90.0f, -90.0f, 45.0f, -45.0f, 135.0f, -135.0f };
	const float CameraOffset = 600.0f;
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(DeathCamera), true, GetPawn());

	FHitResult HitResult;
	for (int32 i = 0; i < UE_ARRAY_COUNT(YawOffsets); i++)
	{
		FRotator CameraDir = ViewDir;
		CameraDir.Yaw += YawOffsets[i];
		CameraDir.Normalize();

		const FVector TestLocation = PawnLocation - CameraDir.Vector() * CameraOffset;
		
		const bool bBlocked = GetWorld()->LineTraceSingleByChannel(HitResult, PawnLocation, TestLocation, ECC_Camera, TraceParams);

		if (!bBlocked)
		{
			CameraLocation = TestLocation;
			CameraRotation = CameraDir;
			return true;
		}
	}

	return false;
}

bool ASoldierPlayerController::ServerCheat_Validate(const FString& Msg)
{
	return true;
}

void ASoldierPlayerController::ServerCheat_Implementation(const FString& Msg)
{
	if (CheatManager)
	{
		ClientMessage(ConsoleCommand(Msg));
	}
}

void ASoldierPlayerController::SimulateInputKey(FKey Key, bool bPressed)
{
	InputKey(Key, bPressed ? IE_Pressed : IE_Released, 1, false);
}

void ASoldierPlayerController::OnKill()
{
	UpdateAchievementProgress(ACH_FRAG_SOMEONE, 100.0f);

	const UWorld* World = GetWorld();

	const IOnlineEventsPtr Events = Online::GetEventsInterface(World);
	const IOnlineIdentityPtr Identity = Online::GetIdentityInterface(World);

	if (Events.IsValid() && Identity.IsValid())
	{
		ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
		if (LocalPlayer)
		{
			int32 UserIndex = LocalPlayer->GetControllerId();
			TSharedPtr<const FUniqueNetId> UniqueID = Identity->GetUniquePlayerId(UserIndex);			
			if (UniqueID.IsValid())
			{			
				ASoldierCharacter* SoldierChar = Cast<ASoldierCharacter>(GetCharacter());
				// If player is dead, use location stored during pawn cleanup.
				FVector Location = SoldierChar ? SoldierChar->GetActorLocation() : LastDeathLocation;
				ASoldierWeapon* Weapon = SoldierChar ? SoldierChar->GetWeapon() : 0;
				int32 WeaponType = Weapon ? (int32)Weapon->GetAmmoType() : 0;

				FOnlineEventParms Params;		

				Params.Add( TEXT( "SectionId" ), FVariantData( (int32)0 ) ); // unused
				Params.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
				Params.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused

				Params.Add( TEXT( "PlayerRoleId" ), FVariantData( (int32)0 ) ); // unused
				Params.Add( TEXT( "PlayerWeaponId" ), FVariantData( (int32)WeaponType ) );
				Params.Add( TEXT( "EnemyRoleId" ), FVariantData( (int32)0 ) ); // unused
				Params.Add( TEXT( "EnemyWeaponId" ), FVariantData( (int32)0 ) ); // untracked			
				Params.Add( TEXT( "KillTypeId" ), FVariantData( (int32)0 ) ); // unused
				Params.Add( TEXT( "LocationX" ), FVariantData( Location.X ) );
				Params.Add( TEXT( "LocationY" ), FVariantData( Location.Y ) );
				Params.Add( TEXT( "LocationZ" ), FVariantData( Location.Z ) );
			
				Events->TriggerEvent(*UniqueID, TEXT("KillOponent"), Params);				
			}
		}
	}
}

void ASoldierPlayerController::OnDeathMessage(class ASoldierPlayerState* KillerPlayerState, class ASoldierPlayerState* KilledPlayerState, const UDamageType* KillerDamageType) 
{
	ASoldierHUD* SoldierHUD = GetSoldierHUD();
	if (SoldierHUD)
	{
		SoldierHUD->ShowDeathMessage(KillerPlayerState, KilledPlayerState, KillerDamageType);		
	}

	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	if (LocalPlayer && LocalPlayer->GetCachedUniqueNetId().IsValid() && KilledPlayerState->UniqueId.IsValid())
	{
		// if this controller is the player who died, update the hero stat.
		if (*LocalPlayer->GetCachedUniqueNetId() == *KilledPlayerState->UniqueId)
		{
			const UWorld* World = GetWorld();
			const IOnlineEventsPtr Events = Online::GetEventsInterface(World);
			const IOnlineIdentityPtr Identity = Online::GetIdentityInterface(World);

			if (Events.IsValid() && Identity.IsValid())
			{							
				const int32 UserIndex = LocalPlayer->GetControllerId();
				TSharedPtr<const FUniqueNetId> UniqueID = Identity->GetUniquePlayerId(UserIndex);
				if (UniqueID.IsValid())
				{				
					ASoldierCharacter* SoldierChar = Cast<ASoldierCharacter>(GetCharacter());
					ASoldierWeapon* Weapon = SoldierChar ? SoldierChar->GetWeapon() : NULL;

					FVector Location = SoldierChar ? SoldierChar->GetActorLocation() : FVector::ZeroVector;
					int32 WeaponType = Weapon ? (int32)Weapon->GetAmmoType() : 0;

					FOnlineEventParms Params;
					Params.Add( TEXT( "SectionId" ), FVariantData( (int32)0 ) ); // unused
					Params.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
					Params.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused

					Params.Add( TEXT( "PlayerRoleId" ), FVariantData( (int32)0 ) ); // unused
					Params.Add( TEXT( "PlayerWeaponId" ), FVariantData( (int32)WeaponType ) );
					Params.Add( TEXT( "EnemyRoleId" ), FVariantData( (int32)0 ) ); // unused
					Params.Add( TEXT( "EnemyWeaponId" ), FVariantData( (int32)0 ) ); // untracked
				
					Params.Add( TEXT( "LocationX" ), FVariantData( Location.X ) );
					Params.Add( TEXT( "LocationY" ), FVariantData( Location.Y ) );
					Params.Add( TEXT( "LocationZ" ), FVariantData( Location.Z ) );
										
					Events->TriggerEvent(*UniqueID, TEXT("PlayerDeath"), Params);
				}
			}
		}
	}	
}

void ASoldierPlayerController::UpdateAchievementProgress( const FString& Id, float Percent )
{
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	if (LocalPlayer)
	{
		IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
		if(OnlineSub)
		{
			IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
			if (Identity.IsValid())
			{
				FUniqueNetIdRepl UserId = LocalPlayer->GetCachedUniqueNetId();

				if (UserId.IsValid())
				{

					IOnlineAchievementsPtr Achievements = OnlineSub->GetAchievementsInterface();
					if (Achievements.IsValid() && (!WriteObject.IsValid() || WriteObject->WriteState != EOnlineAsyncTaskState::InProgress))
					{
						WriteObject = MakeShareable(new FOnlineAchievementsWrite());
						WriteObject->SetFloatStat(*Id, Percent);

						FOnlineAchievementsWriteRef WriteObjectRef = WriteObject.ToSharedRef();
						Achievements->WriteAchievements(*UserId, WriteObjectRef);
					}
					else
					{
						UE_LOG(LogOnline, Warning, TEXT("No valid achievement interface or another write is in progress."));
					}
				}
				else
				{
					UE_LOG(LogOnline, Warning, TEXT("No valid user id for this controller."));
				}
			}
			else
			{
				UE_LOG(LogOnline, Warning, TEXT("No valid identity interface."));
			}
		}
		else
		{
			UE_LOG(LogOnline, Warning, TEXT("No default online subsystem."));
		}
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("No local player, cannot update achievements."));
	}
}

void ASoldierPlayerController::OnToggleInGameMenu()
{
	if( GEngine->GameViewport == nullptr )
	{
		return;
	}

	// this is not ideal, but necessary to prevent both players from pausing at the same time on the same frame
	UWorld* GameWorld = GEngine->GameViewport->GetWorld();

	for(auto It = GameWorld->GetControllerIterator(); It; ++It)
	{
		ASoldierPlayerController* Controller = Cast<ASoldierPlayerController>(*It);
		if(Controller && Controller->IsPaused())
		{
			return;
		}
	}

	// if no one's paused, pause
	if (SoldierIngameMenu.IsValid())
	{
		SoldierIngameMenu->ToggleGameMenu();
	}
}

void ASoldierPlayerController::OnConditionalCloseScoreboard()
{
	ASoldierHUD* SoldierHUD = GetSoldierHUD();
	if(SoldierHUD && ( SoldierHUD->IsMatchOver() == false ))
	{
		SoldierHUD->ConditionalCloseScoreboard();
	}
}

void ASoldierPlayerController::OnToggleScoreboard()
{
	ASoldierHUD* SoldierHUD = GetSoldierHUD();
	if(SoldierHUD && ( SoldierHUD->IsMatchOver() == false ))
	{
		SoldierHUD->ToggleScoreboard();
	}
}

void ASoldierPlayerController::OnShowScoreboard()
{
	ASoldierHUD* SoldierHUD = GetSoldierHUD();
	if(SoldierHUD)
	{
		SoldierHUD->ShowScoreboard(true);
	}
}

void ASoldierPlayerController::OnHideScoreboard()
{
	ASoldierHUD* SoldierHUD = GetSoldierHUD();
	// If have a valid match and the match is over - hide the scoreboard
	if( (SoldierHUD != NULL ) && ( SoldierHUD->IsMatchOver() == false ) )
	{
		SoldierHUD->ShowScoreboard(false);
	}
}

bool ASoldierPlayerController::IsGameMenuVisible() const
{
	bool Result = false; 
	if (SoldierIngameMenu.IsValid())
	{
		Result = SoldierIngameMenu->GetIsGameMenuUp();
	} 

	return Result;
}

void ASoldierPlayerController::SetInfiniteAmmo(bool bEnable)
{
	bInfiniteAmmo = bEnable;
}

void ASoldierPlayerController::SetInfiniteClip(bool bEnable)
{
	bInfiniteClip = bEnable;
}

void ASoldierPlayerController::SetHealthRegen(bool bEnable)
{
	bHealthRegen = bEnable;
}

void ASoldierPlayerController::SetGodMode(bool bEnable)
{
	bGodMode = bEnable;
}

void ASoldierPlayerController::SetIsVibrationEnabled(bool bEnable)
{
	bIsVibrationEnabled = bEnable;
}

void ASoldierPlayerController::ClientGameStarted_Implementation()
{
	bAllowGameActions = true;

	// Enable controls mode now the game has started
	SetIgnoreMoveInput(false);

	ASoldierHUD* SoldierHUD = GetSoldierHUD();
	if (SoldierHUD)
	{
		SoldierHUD->SetMatchState(ESoldierMatchState::Playing);
		SoldierHUD->ShowScoreboard(false);
	}
	bGameEndedFrame = false;

	QueryAchievements();

	QueryStats();

	const UWorld* World = GetWorld();

	// Send round start event
	const IOnlineEventsPtr Events = Online::GetEventsInterface(World);
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);

	if(LocalPlayer != nullptr && World != nullptr && Events.IsValid())
	{
		FUniqueNetIdRepl UniqueId = LocalPlayer->GetPreferredUniqueNetId();

		if (UniqueId.IsValid())
		{
			// Generate a new session id
			Events->SetPlayerSessionId(*UniqueId, FGuid::NewGuid());

			FString MapName = *FPackageName::GetShortName(World->PersistentLevel->GetOutermost()->GetName());

			// Fire session start event for all cases
			FOnlineEventParms Params;
			Params.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
			Params.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused
			Params.Add( TEXT( "MapName" ), FVariantData( MapName ) );
			
			Events->TriggerEvent(*UniqueId, TEXT("PlayerSessionStart"), Params);

			// Online matches require the MultiplayerRoundStart event as well
			USoldierGameInstance* SGI = Cast<USoldierGameInstance>(World->GetGameInstance());

			if (SGI && (SGI->GetOnlineMode() == EOnlineMode::Online))
			{
				FOnlineEventParms MultiplayerParams;

				// @todo: fill in with real values
				MultiplayerParams.Add( TEXT( "SectionId" ), FVariantData( (int32)0 ) ); // unused
				MultiplayerParams.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
				MultiplayerParams.Add( TEXT( "MatchTypeId" ), FVariantData( (int32)1 ) ); // @todo abstract the specific meaning of this value across platforms
				MultiplayerParams.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused
				
				Events->TriggerEvent(*UniqueId, TEXT("MultiplayerRoundStart"), MultiplayerParams);
			}

			bHasSentStartEvents = true;
		}
	}
}

/** Starts the online game using the session name in the PlayerState */
void ASoldierPlayerController::ClientStartOnlineGame_Implementation()
{
	if (!IsPrimaryPlayer())
		return;

	ASoldierPlayerState* SoldierPlayerState = Cast<ASoldierPlayerState>(PlayerState);
	if (SoldierPlayerState)
	{
		IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
		if (OnlineSub)
		{
			IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
			if (Sessions.IsValid() && (Sessions->GetNamedSession(SoldierPlayerState->SessionName) != nullptr))
			{
				UE_LOG(LogOnline, Log, TEXT("Starting session %s on client"), *SoldierPlayerState->SessionName.ToString() );
				Sessions->StartSession(SoldierPlayerState->SessionName);
			}
		}
	}
	else
	{
		// Keep retrying until player state is replicated
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_ClientStartOnlineGame, this, &ASoldierPlayerController::ClientStartOnlineGame_Implementation, 0.2f, false);
	}
}

/** Ends the online game using the session name in the PlayerState */
void ASoldierPlayerController::ClientEndOnlineGame_Implementation()
{
	if (!IsPrimaryPlayer())
		return;

	ASoldierPlayerState* SoldierPlayerState = Cast<ASoldierPlayerState>(PlayerState);
	if (SoldierPlayerState)
	{
		IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
		if (OnlineSub)
		{
			IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
			if (Sessions.IsValid() && (Sessions->GetNamedSession(SoldierPlayerState->SessionName) != nullptr))
			{
				UE_LOG(LogOnline, Log, TEXT("Ending session %s on client"), *SoldierPlayerState->SessionName.ToString() );
				Sessions->EndSession(SoldierPlayerState->SessionName);
			}
		}
	}
}

void ASoldierPlayerController::HandleReturnToMainMenu()
{
	OnHideScoreboard();
	CleanupSessionOnReturnToMenu();
}

void ASoldierPlayerController::ClientReturnToMainMenu_Implementation(const FString& InReturnReason)
{		
	const UWorld* World = GetWorld();
	USoldierGameInstance* SGI = World != NULL ? Cast<USoldierGameInstance>(World->GetGameInstance()) : NULL;

	if ( !ensure( SGI != NULL ) )
	{
		return;
	}

	if ( GetNetMode() == NM_Client )
	{
		const FText ReturnReason	= NSLOCTEXT( "NetworkErrors", "HostQuit", "The host has quit the match." );
		const FText OKButton		= NSLOCTEXT( "DialogButtons", "OKAY", "OK" );

		SGI->ShowMessageThenGotoState( ReturnReason, OKButton, FText::GetEmpty(), SoldierGameInstanceState::MainMenu );
	}
	else
	{
		SGI->GotoState(SoldierGameInstanceState::MainMenu);
	}

	// Clear the flag so we don't do normal end of round stuff next
	bGameEndedFrame = false;
}

/** Ends and/or destroys game session */
void ASoldierPlayerController::CleanupSessionOnReturnToMenu()
{
	const UWorld* World = GetWorld();
	USoldierGameInstance * SGI = World != NULL ? Cast<USoldierGameInstance>( World->GetGameInstance() ) : NULL;

	if ( ensure( SGI != NULL ) )
	{
		SGI->CleanupSessionOnReturnToMenu();
	}
}

void ASoldierPlayerController::ClientGameEnded_Implementation(class AActor* EndGameFocus, bool bIsWinner)
{
	Super::ClientGameEnded_Implementation(EndGameFocus, bIsWinner);
	
	// Disable controls now the game has ended
	SetIgnoreMoveInput(true);

	bAllowGameActions = false;

	// Make sure that we still have valid view target
	SetViewTarget(GetPawn());

	ASoldierHUD* SoldierHUD = GetSoldierHUD();
	if (SoldierHUD)
	{
		SoldierHUD->SetMatchState(bIsWinner ? ESoldierMatchState::Won : ESoldierMatchState::Lost);
	}

	UpdateSaveFileOnGameEnd(bIsWinner);
	UpdateAchievementsOnGameEnd();
	UpdateLeaderboardsOnGameEnd();
	UpdateStatsOnGameEnd(bIsWinner);

	// Flag that the game has just ended (if it's ended due to host loss we want to wait for ClientReturnToMainMenu_Implementation first, incase we don't want to process)
	bGameEndedFrame = true;
}

void ASoldierPlayerController::ClientSendRoundEndEvent_Implementation(bool bIsWinner, int32 ExpendedTimeInSeconds)
{
	const UWorld* World = GetWorld();
	const IOnlineEventsPtr Events = Online::GetEventsInterface(World);
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);

	if(bHasSentStartEvents && LocalPlayer != nullptr && World != nullptr && Events.IsValid())
	{	
		FUniqueNetIdRepl UniqueId = LocalPlayer->GetPreferredUniqueNetId();

		if (UniqueId.IsValid())
		{
			FString MapName = *FPackageName::GetShortName(World->PersistentLevel->GetOutermost()->GetName());
			ASoldierPlayerState* SoldierPlayerState = Cast<ASoldierPlayerState>(PlayerState);
			int32 PlayerScore = SoldierPlayerState ? SoldierPlayerState->GetScore() : 0;
			
			// Fire session end event for all cases
			FOnlineEventParms Params;
			Params.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
			Params.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused
			Params.Add( TEXT( "ExitStatusId" ), FVariantData( (int32)0 ) ); // unused
			Params.Add( TEXT( "PlayerScore" ), FVariantData( (int32)PlayerScore ) );
			Params.Add( TEXT( "PlayerWon" ), FVariantData( (bool)bIsWinner ) );
			Params.Add( TEXT( "MapName" ), FVariantData( MapName ) );
			Params.Add( TEXT( "MapNameString" ), FVariantData( MapName ) ); // @todo workaround for a bug in backend service, remove when fixed
			
			Events->TriggerEvent(*UniqueId, TEXT("PlayerSessionEnd"), Params);

			// Online matches require the MultiplayerRoundEnd event as well
			USoldierGameInstance* SGI = Cast<USoldierGameInstance>(World->GetGameInstance());
			if (SGI && (SGI->GetOnlineMode() == EOnlineMode::Online))
			{
				FOnlineEventParms MultiplayerParams;

				ASoldierGameState* const MyGameState = World->GetGameState<ASoldierGameState>();
				if (ensure(MyGameState != nullptr))
				{
					MultiplayerParams.Add( TEXT( "SectionId" ), FVariantData( (int32)0 ) ); // unused
					MultiplayerParams.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
					MultiplayerParams.Add( TEXT( "MatchTypeId" ), FVariantData( (int32)1 ) ); // @todo abstract the specific meaning of this value across platforms
					MultiplayerParams.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused
					MultiplayerParams.Add( TEXT( "TimeInSeconds" ), FVariantData( (float)ExpendedTimeInSeconds ) );
					MultiplayerParams.Add( TEXT( "ExitStatusId" ), FVariantData( (int32)0 ) ); // unused
					
					Events->TriggerEvent(*UniqueId, TEXT("MultiplayerRoundEnd"), MultiplayerParams);
				}
			}
		}

		bHasSentStartEvents = false;
	}
}

void ASoldierPlayerController::SetCinematicMode(bool bInCinematicMode, bool bHidePlayer, bool bAffectsHUD, bool bAffectsMovement, bool bAffectsTurning)
{
	Super::SetCinematicMode(bInCinematicMode, bHidePlayer, bAffectsHUD, bAffectsMovement, bAffectsTurning);

	// If we have a pawn we need to determine if we should show/hide the weapon
	ASoldierCharacter* MyPawn = Cast<ASoldierCharacter>(GetPawn());
	ASoldierWeapon* MyWeapon = MyPawn ? MyPawn->GetWeapon() : NULL;
	if (MyWeapon)
	{
		if (bInCinematicMode && bHidePlayer)
		{
			MyWeapon->SetActorHiddenInGame(true);
		}
		else if (!bCinematicMode)
		{
			MyWeapon->SetActorHiddenInGame(false);
		}
	}
}

bool ASoldierPlayerController::IsMoveInputIgnored() const
{
	if (IsInState(NAME_Spectating))
	{
		return false;
	}
	else
	{
		return Super::IsMoveInputIgnored();
	}
}

bool ASoldierPlayerController::IsLookInputIgnored() const
{
	if (IsInState(NAME_Spectating))
	{
		return false;
	}
	else
	{
		return Super::IsLookInputIgnored();
	}
}

void ASoldierPlayerController::InitInputSystem()
{
	Super::InitInputSystem();

	USoldierPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
		PersistentUser->TellInputAboutKeybindings();
	}
}

void ASoldierPlayerController::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME_CONDITION( ASoldierPlayerController, bInfiniteAmmo, COND_OwnerOnly );
	DOREPLIFETIME_CONDITION( ASoldierPlayerController, bInfiniteClip, COND_OwnerOnly );

	DOREPLIFETIME(ASoldierPlayerController, bHealthRegen);
}

void ASoldierPlayerController::Suicide()
{
	if ( IsInState(NAME_Playing) )
	{
		ServerSuicide();
	}
}

bool ASoldierPlayerController::ServerSuicide_Validate()
{
	return true;
}

void ASoldierPlayerController::ServerSuicide_Implementation()
{
	if ( (GetPawn() != NULL) && ((GetWorld()->TimeSeconds - GetPawn()->CreationTime > 1) || (GetNetMode() == NM_Standalone)) )
	{
		ASoldierCharacter* MyPawn = Cast<ASoldierCharacter>(GetPawn());
		if (MyPawn)
		{
			MyPawn->Suicide();
		}
	}
}

bool ASoldierPlayerController::HasInfiniteAmmo() const
{
	return bInfiniteAmmo;
}

bool ASoldierPlayerController::HasInfiniteClip() const
{
	return bInfiniteClip;
}

bool ASoldierPlayerController::HasHealthRegen() const
{
	return bHealthRegen;
}

bool ASoldierPlayerController::HasGodMode() const
{
	return bGodMode;
}

bool ASoldierPlayerController::IsVibrationEnabled() const
{
	return bIsVibrationEnabled;
}

bool ASoldierPlayerController::IsGameInputAllowed() const
{
	return bAllowGameActions && !bCinematicMode;
}

void ASoldierPlayerController::ToggleChatWindow()
{
	ASoldierHUD* SoldierHUD = Cast<ASoldierHUD>(GetHUD());
	if (SoldierHUD)
	{
		SoldierHUD->ToggleChat();
	}
}

void ASoldierPlayerController::ClientTeamMessage_Implementation( APlayerState* SenderPlayerState, const FString& S, FName Type, float MsgLifeTime  )
{
	ASoldierHUD* SoldierHUD = Cast<ASoldierHUD>(GetHUD());
	if (SoldierHUD)
	{
		if( Type == ServerSayString )
		{
			if( SenderPlayerState != PlayerState  )
			{
				SoldierHUD->AddChatLine(FText::FromString(S), false);
			}
		}
	}
}

void ASoldierPlayerController::Say( const FString& Msg )
{
	ServerSay(Msg.Left(128));
}

bool ASoldierPlayerController::ServerSay_Validate( const FString& Msg )
{
	return true;
}

void ASoldierPlayerController::ServerSay_Implementation( const FString& Msg )
{
	GetWorld()->GetAuthGameMode<ASoldierGameMode>()->Broadcast(this, Msg, ServerSayString);
}

ASoldierHUD* ASoldierPlayerController::GetSoldierHUD() const
{
	return Cast<ASoldierHUD>(GetHUD());
}


USoldierPersistentUser* ASoldierPlayerController::GetPersistentUser() const
{
	USoldierLocalPlayer* const SoldierLocalPlayer = Cast<USoldierLocalPlayer>(Player);
	return SoldierLocalPlayer ? SoldierLocalPlayer->GetPersistentUser() : nullptr;
}

bool ASoldierPlayerController::SetPause(bool bPause, FCanUnpause CanUnpauseDelegate /*= FCanUnpause()*/)
{
	const bool Result = APlayerController::SetPause(bPause, CanUnpauseDelegate);

	// Update rich presence.
	const UWorld* World = GetWorld();
	const IOnlinePresencePtr PresenceInterface = Online::GetPresenceInterface(World);
	const IOnlineEventsPtr Events = Online::GetEventsInterface(World);
	const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	FUniqueNetIdRepl UserId = LocalPlayer ? LocalPlayer->GetCachedUniqueNetId() : FUniqueNetIdRepl();

	// Don't send pause events while online since the game doesn't actually pause
	if(GetNetMode() == NM_Standalone && Events.IsValid() && PlayerState->UniqueId.IsValid())
	{
		FOnlineEventParms Params;
		Params.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
		Params.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused
		if(Result && bPause)
		{
			Events->TriggerEvent(*PlayerState->UniqueId, TEXT("PlayerSessionPause"), Params);
		}
		else
		{
			Events->TriggerEvent(*PlayerState->UniqueId, TEXT("PlayerSessionResume"), Params);
		}
	}

	return Result;
}

FVector ASoldierPlayerController::GetFocalLocation() const
{
	const ASoldierCharacter* SoldierCharacter = Cast<ASoldierCharacter>(GetPawn());

	// On death we want to use the player's death cam location rather than the location of where the pawn is at the moment
	// This guarantees that the clients see their death cam correctly, as their pawns have delayed destruction.
	if (SoldierCharacter && SoldierCharacter->bIsDying)
	{
		return GetSpawnLocation();
	}

	return Super::GetFocalLocation();
}

void ASoldierPlayerController::ShowInGameMenu()
{
	ASoldierHUD* SoldierHUD = GetSoldierHUD();	
	if(SoldierIngameMenu.IsValid() && !SoldierIngameMenu->GetIsGameMenuUp() && SoldierHUD && (SoldierHUD->IsMatchOver() == false))
	{
		SoldierIngameMenu->ToggleGameMenu();
	}
}
void ASoldierPlayerController::UpdateAchievementsOnGameEnd()
{
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	if (LocalPlayer)
	{
		ASoldierPlayerState* SoldierPlayerState = Cast<ASoldierPlayerState>(PlayerState);
		if (SoldierPlayerState)
		{			
			const USoldierPersistentUser*  PersistentUser = GetPersistentUser();

			if (PersistentUser)
			{						
				const int32 Wins = PersistentUser->GetWins();
				const int32 Losses = PersistentUser->GetLosses();
				const int32 Matches = Wins + Losses;

				const int32 TotalKills = PersistentUser->GetKills();
				const int32 MatchScore = (int32)SoldierPlayerState->GetScore();

				const int32 TotalBulletsFired = PersistentUser->GetBulletsFired();
				const int32 TotalRocketsFired = PersistentUser->GetRocketsFired();
			
				float TotalGameAchievement = 0;
				float CurrentGameAchievement = 0;
			
				///////////////////////////////////////
				// Kill achievements
				if (TotalKills >= 1)
				{
					CurrentGameAchievement += 100.0f;
				}
				TotalGameAchievement += 100;

				{
					float fSomeKillPct = ((float)TotalKills / (float)SomeKillsCount) * 100.0f;
					fSomeKillPct = FMath::RoundToFloat(fSomeKillPct);
					UpdateAchievementProgress(ACH_SOME_KILLS, fSomeKillPct);

					CurrentGameAchievement += FMath::Min(fSomeKillPct, 100.0f);
					TotalGameAchievement += 100;
				}

				{
					float fLotsKillPct = ((float)TotalKills / (float)LotsKillsCount) * 100.0f;
					fLotsKillPct = FMath::RoundToFloat(fLotsKillPct);
					UpdateAchievementProgress(ACH_LOTS_KILLS, fLotsKillPct);

					CurrentGameAchievement += FMath::Min(fLotsKillPct, 100.0f);
					TotalGameAchievement += 100;
				}
				///////////////////////////////////////

				///////////////////////////////////////
				// Match Achievements
				{
					UpdateAchievementProgress(ACH_FINISH_MATCH, 100.0f);

					CurrentGameAchievement += 100;
					TotalGameAchievement += 100;
				}
			
				{
					float fLotsRoundsPct = ((float)Matches / (float)LotsMatchesCount) * 100.0f;
					fLotsRoundsPct = FMath::RoundToFloat(fLotsRoundsPct);
					UpdateAchievementProgress(ACH_LOTS_MATCHES, fLotsRoundsPct);

					CurrentGameAchievement += FMath::Min(fLotsRoundsPct, 100.0f);
					TotalGameAchievement += 100;
				}
				///////////////////////////////////////

				///////////////////////////////////////
				// Win Achievements
				if (Wins >= 1)
				{
					UpdateAchievementProgress(ACH_FIRST_WIN, 100.0f);

					CurrentGameAchievement += 100.0f;
				}
				TotalGameAchievement += 100;

				{			
					float fLotsWinPct = ((float)Wins / (float)LotsWinsCount) * 100.0f;
					fLotsWinPct = FMath::RoundToInt(fLotsWinPct);
					UpdateAchievementProgress(ACH_LOTS_WIN, fLotsWinPct);

					CurrentGameAchievement += FMath::Min(fLotsWinPct, 100.0f);
					TotalGameAchievement += 100;
				}

				{			
					float fManyWinPct = ((float)Wins / (float)ManyWinsCount) * 100.0f;
					fManyWinPct = FMath::RoundToInt(fManyWinPct);
					UpdateAchievementProgress(ACH_MANY_WIN, fManyWinPct);

					CurrentGameAchievement += FMath::Min(fManyWinPct, 100.0f);
					TotalGameAchievement += 100;
				}
				///////////////////////////////////////

				///////////////////////////////////////
				// Ammo Achievements
				{
					float fLotsBulletsPct = ((float)TotalBulletsFired / (float)LotsBulletsCount) * 100.0f;
					fLotsBulletsPct = FMath::RoundToFloat(fLotsBulletsPct);
					UpdateAchievementProgress(ACH_SHOOT_BULLETS, fLotsBulletsPct);

					CurrentGameAchievement += FMath::Min(fLotsBulletsPct, 100.0f);
					TotalGameAchievement += 100;
				}

				{
					float fLotsRocketsPct = ((float)TotalRocketsFired / (float)LotsRocketsCount) * 100.0f;
					fLotsRocketsPct = FMath::RoundToFloat(fLotsRocketsPct);
					UpdateAchievementProgress(ACH_SHOOT_ROCKETS, fLotsRocketsPct);

					CurrentGameAchievement += FMath::Min(fLotsRocketsPct, 100.0f);
					TotalGameAchievement += 100;
				}
				///////////////////////////////////////

				///////////////////////////////////////
				// Score Achievements
				{
					float fGoodScorePct = ((float)MatchScore / (float)GoodScoreCount) * 100.0f;
					fGoodScorePct = FMath::RoundToFloat(fGoodScorePct);
					UpdateAchievementProgress(ACH_GOOD_SCORE, fGoodScorePct);
				}

				{
					float fGreatScorePct = ((float)MatchScore / (float)GreatScoreCount) * 100.0f;
					fGreatScorePct = FMath::RoundToFloat(fGreatScorePct);
					UpdateAchievementProgress(ACH_GREAT_SCORE, fGreatScorePct);
				}
				///////////////////////////////////////

				///////////////////////////////////////
				// Map Play Achievements
				UWorld* World = GetWorld();
				if (World)
				{			
					FString MapName = *FPackageName::GetShortName(World->PersistentLevel->GetOutermost()->GetName());
					if (MapName.Find(TEXT("Highrise")) != -1)
					{
						UpdateAchievementProgress(ACH_PLAY_HIGHRISE, 100.0f);
					}
					else if (MapName.Find(TEXT("Sanctuary")) != -1)
					{
						UpdateAchievementProgress(ACH_PLAY_SANCTUARY, 100.0f);
					}
				}
				///////////////////////////////////////			

				const IOnlineEventsPtr Events = Online::GetEventsInterface(World);
				const IOnlineIdentityPtr Identity = Online::GetIdentityInterface(World);

				if (Events.IsValid() && Identity.IsValid())
				{							
					const int32 UserIndex = LocalPlayer->GetControllerId();
					TSharedPtr<const FUniqueNetId> UniqueID = Identity->GetUniquePlayerId(UserIndex);
					if (UniqueID.IsValid())
					{				
						FOnlineEventParms Params;

						float fGamePct = (CurrentGameAchievement / TotalGameAchievement) * 100.0f;
						fGamePct = FMath::RoundToFloat(fGamePct);
						Params.Add( TEXT( "CompletionPercent" ), FVariantData( (float)fGamePct ) );
						if (UniqueID.IsValid())
						{				
							Events->TriggerEvent(*UniqueID, TEXT("GameProgress"), Params);
						}
					}
				}
			}
		}
	}
}

void ASoldierPlayerController::UpdateLeaderboardsOnGameEnd()
{
	USoldierLocalPlayer* LocalPlayer = Cast<USoldierLocalPlayer>(Player);
	if (LocalPlayer)
	{
		// update leaderboards - note this does not respect existing scores and overwrites them. We would first need to read the leaderboards if we wanted to do that.
		IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetWorld());
		if (OnlineSub)
		{
			IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
			if (Identity.IsValid())
			{
				TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(LocalPlayer->GetControllerId());
				if (UserId.IsValid())
				{
					IOnlineLeaderboardsPtr Leaderboards = OnlineSub->GetLeaderboardsInterface();
					if (Leaderboards.IsValid())
					{
						ASoldierPlayerState* SoldierPlayerState = Cast<ASoldierPlayerState>(PlayerState);
						if (SoldierPlayerState)
						{
							FSoldierAllTimeMatchResultsWrite ResultsWriteObject;
							int32 MatchWriteData = 1;
							int32 KillsWriteData = SoldierPlayerState->GetKills();
							int32 DeathsWriteData = SoldierPlayerState->GetDeaths();

#if !PLATFORM_XBOXONE
							StatMatchesPlayed = (MatchWriteData += StatMatchesPlayed);
							StatKills = (KillsWriteData += StatKills);
							StatDeaths = (DeathsWriteData += StatDeaths);
#endif

							ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_SCORE, KillsWriteData);
							ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_KILLS, KillsWriteData);
							ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_DEATHS, DeathsWriteData);
							ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_MATCHESPLAYED, MatchWriteData);

							// the call will copy the user id and write object to its own memory
							Leaderboards->WriteLeaderboards(SoldierPlayerState->SessionName, *UserId, ResultsWriteObject);
							Leaderboards->FlushLeaderboards(TEXT("SHOOTERGAME"));
						}
					}
				}
			}
		}
	}
}

void ASoldierPlayerController::UpdateStatsOnGameEnd(bool bIsWinner)
{
	const IOnlineStatsPtr Stats = Online::GetStatsInterface(GetWorld());
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	ASoldierPlayerState* SoldierPlayerState = Cast<ASoldierPlayerState>(PlayerState);

	if (Stats.IsValid() && LocalPlayer != nullptr && SoldierPlayerState != nullptr)
	{
		FUniqueNetIdRepl UniqueId = LocalPlayer->GetCachedUniqueNetId();

		if (UniqueId.IsValid() )
		{
			TArray<FOnlineStatsUserUpdatedStats> UpdatedUserStats;

			FOnlineStatsUserUpdatedStats& UpdatedStats = UpdatedUserStats.Emplace_GetRef( UniqueId.GetUniqueNetId().ToSharedRef() );
			UpdatedStats.Stats.Add( TEXT("Kills"), FOnlineStatUpdate( SoldierPlayerState->GetKills(), FOnlineStatUpdate::EOnlineStatModificationType::Sum ) );
			UpdatedStats.Stats.Add( TEXT("Deaths"), FOnlineStatUpdate( SoldierPlayerState->GetDeaths(), FOnlineStatUpdate::EOnlineStatModificationType::Sum ) );
			UpdatedStats.Stats.Add( TEXT("RoundsPlayed"), FOnlineStatUpdate( 1, FOnlineStatUpdate::EOnlineStatModificationType::Sum ) );
			if (bIsWinner)
			{
				UpdatedStats.Stats.Add( TEXT("RoundsWon"), FOnlineStatUpdate( 1, FOnlineStatUpdate::EOnlineStatModificationType::Sum ) );
			}

			Stats->UpdateStats( UniqueId.GetUniqueNetId().ToSharedRef(), UpdatedUserStats, FOnlineStatsUpdateStatsComplete() );
		}
	}
}


void ASoldierPlayerController::UpdateSaveFileOnGameEnd(bool bIsWinner)
{
	ASoldierPlayerState* SoldierPlayerState = Cast<ASoldierPlayerState>(PlayerState);
	if (SoldierPlayerState)
	{
		// update local saved profile
		USoldierPersistentUser* const PersistentUser = GetPersistentUser();
		if (PersistentUser)
		{
			PersistentUser->AddMatchResult(SoldierPlayerState->GetKills(), SoldierPlayerState->GetDeaths(), SoldierPlayerState->GetNumBulletsFired(), SoldierPlayerState->GetNumRocketsFired(), bIsWinner);
			PersistentUser->SaveIfDirty();
		}
	}
}

void ASoldierPlayerController::PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel)
{
	Super::PreClientTravel( PendingURL, TravelType, bIsSeamlessTravel );

	if (const UWorld* World = GetWorld())
	{
		USoldierGameViewportClient* SoldierViewport = Cast<USoldierGameViewportClient>( World->GetGameViewport() );

		if ( SoldierViewport != NULL )
		{
			SoldierViewport->ShowLoadingScreen();
		}
		
		ASoldierHUD* SoldierHUD = Cast<ASoldierHUD>(GetHUD());
		if (SoldierHUD != nullptr)
		{
			// Passing true to bFocus here ensures that focus is returned to the game viewport.
			SoldierHUD->ShowScoreboard(false, true);
		}
	}
}

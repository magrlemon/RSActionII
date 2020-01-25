// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
#include "Online/SoldierGameMode.h"
#include "RSAction.h"
#include "SoldierGameInstance.h"
#include "UI/SoldierHUD.h"
#include "Player/SoldierSpectatorPawn.h"
#include "Player/SoldierDemoSpectator.h"
#include "Online/SoldierPlayerState.h"
#include "Online/SoldierGameSession.h"
#include "Bots/SoldierAIController.h"
#include "SoldierTeamStart.h"


ASoldierGameMode::ASoldierGameMode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnOb(TEXT("/Game/Blueprints/Pawns/SoldierPawn"));
	DefaultPawnClass = PlayerPawnOb.Class;
	
	static ConstructorHelpers::FClassFinder<APawn> BotPawnOb(TEXT("/Game/Blueprints/Pawns/BotPawn"));
	BotPawnClass = BotPawnOb.Class;

	HUDClass = ASoldierHUD::StaticClass();
	PlayerControllerClass = ASoldierPlayerController::StaticClass();
	PlayerStateClass = ASoldierPlayerState::StaticClass();
	SpectatorClass = ASoldierSpectatorPawn::StaticClass();
	GameStateClass = ASoldierGameState::StaticClass();
	ReplaySpectatorPlayerControllerClass = ASoldierDemoSpectator::StaticClass();

	MinRespawnDelay = 5.0f;

	bAllowBots = true;	
	bNeedsBotCreation = true;
	bUseSeamlessTravel = FParse::Param(FCommandLine::Get(), TEXT("NoSeamlessTravel")) ? false : true;
}

void ASoldierGameMode::PostInitProperties()
{
	Super::PostInitProperties();
	if (PlatformPlayerControllerClass != nullptr)
	{
		PlayerControllerClass = PlatformPlayerControllerClass;
	}
}

FString ASoldierGameMode::GetBotsCountOptionName()
{
	return FString(TEXT("Bots"));
}

void ASoldierGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	const int32 BotsCountOptionValue = UGameplayStatics::GetIntOption(Options, GetBotsCountOptionName(), 0);
	SetAllowBots(BotsCountOptionValue > 0 ? true : false, BotsCountOptionValue);	
	Super::InitGame(MapName, Options, ErrorMessage);

	const UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance && Cast<USoldierGameInstance>(GameInstance)->GetOnlineMode() != EOnlineMode::Offline)
	{
		bPauseable = false;
	}
}

void ASoldierGameMode::SetAllowBots(bool bInAllowBots, int32 InMaxBots)
{
	bAllowBots = bInAllowBots;
	MaxBots = InMaxBots;
}

/** Returns game session class to use */
TSubclassOf<AGameSession> ASoldierGameMode::GetGameSessionClass() const
{
	return ASoldierGameSession::StaticClass();
}

void ASoldierGameMode::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	GetWorldTimerManager().SetTimer(TimerHandle_DefaultTimer, this, &ASoldierGameMode::DefaultTimer, GetWorldSettings()->GetEffectiveTimeDilation(), true);
}

void ASoldierGameMode::DefaultTimer()
{
	// don't update timers for Play In Editor mode, it's not real match
	if (GetWorld()->IsPlayInEditor())
	{
		// start match if necessary.
		if (GetMatchState() == MatchState::WaitingToStart)
		{
			StartMatch();
		}
		return;
	}

	ASoldierGameState* const MyGameState = Cast<ASoldierGameState>(GameState);
	if (MyGameState && MyGameState->RemainingTime > 0 && !MyGameState->bTimerPaused)
	{
		MyGameState->RemainingTime--;
		
		if (MyGameState->RemainingTime <= 0)
		{
			if (GetMatchState() == MatchState::WaitingPostMatch)
			{
				RestartGame();
			}
			else if (GetMatchState() == MatchState::InProgress)
			{
				FinishMatch();

				// Send end round events
				for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
				{
					ASoldierPlayerController* PlayerController = Cast<ASoldierPlayerController>(*It);
					
					if (PlayerController && MyGameState)
					{
						ASoldierPlayerState* PlayerState = Cast<ASoldierPlayerState>((*It)->PlayerState);
						const bool bIsWinner = IsWinner(PlayerState);
					
						PlayerController->ClientSendRoundEndEvent(bIsWinner, MyGameState->ElapsedTime);
					}
				}
			}
			else if (GetMatchState() == MatchState::WaitingToStart)
			{
				StartMatch();
			}
		}
	}
}

void ASoldierGameMode::HandleMatchIsWaitingToStart()
{
	Super::HandleMatchIsWaitingToStart();

	if (bNeedsBotCreation)
	{
		CreateBotControllers();
		bNeedsBotCreation = false;
	}

	if (bDelayedStart)
	{
		// start warmup if needed
		ASoldierGameState* const MyGameState = Cast<ASoldierGameState>(GameState);
		if (MyGameState && MyGameState->RemainingTime == 0)
		{
			const bool bWantsMatchWarmup = !GetWorld()->IsPlayInEditor();
			if (bWantsMatchWarmup && WarmupTime > 0)
			{
				MyGameState->RemainingTime = WarmupTime;
			}
			else
			{
				MyGameState->RemainingTime = 0.0f;
			}
		}
	}
}

void ASoldierGameMode::HandleMatchHasStarted()
{
	bNeedsBotCreation = true;
	Super::HandleMatchHasStarted();

	ASoldierGameState* const MyGameState = Cast<ASoldierGameState>(GameState);
	MyGameState->RemainingTime = RoundTime;	
	StartBots();	

	// notify players
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		ASoldierPlayerController* PC = Cast<ASoldierPlayerController>(*It);
		if (PC)
		{
			PC->ClientGameStarted();
		}
	}
}

void ASoldierGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

void ASoldierGameMode::FinishMatch()
{
	ASoldierGameState* const MyGameState = Cast<ASoldierGameState>(GameState);
	if (IsMatchInProgress())
	{
		EndMatch();
		DetermineMatchWinner();		

		// notify players
		for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
		{
			ASoldierPlayerState* PlayerState = Cast<ASoldierPlayerState>((*It)->PlayerState);
			const bool bIsWinner = IsWinner(PlayerState);

			(*It)->GameHasEnded(NULL, bIsWinner);
		}

		// lock all pawns
		// pawns are not marked as keep for seamless travel, so we will create new pawns on the next match rather than
		// turning these back on.
		for (APawn* Pawn : TActorRange<APawn>(GetWorld()))
		{
			Pawn->TurnOff();
		}

		// set up to restart the match
		MyGameState->RemainingTime = TimeBetweenMatches;
	}
}

void ASoldierGameMode::RequestFinishAndExitToMainMenu()
{
	FinishMatch();

	USoldierGameInstance* const GameInstance = Cast<USoldierGameInstance>(GetGameInstance());
	if (GameInstance)
	{
		GameInstance->RemoveSplitScreenPlayers();
	}

	ASoldierPlayerController* LocalPrimaryController = nullptr;
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		ASoldierPlayerController* Controller = Cast<ASoldierPlayerController>(*Iterator);

		if (Controller == NULL)
		{
			continue;
		}

		if (!Controller->IsLocalController())
		{
			const FText RemoteReturnReason = NSLOCTEXT("NetworkErrors", "HostHasLeft", "Host has left the game.");
			Controller->ClientReturnToMainMenuWithTextReason(RemoteReturnReason);
		}
		else
		{
			LocalPrimaryController = Controller;
		}
	}

	// GameInstance should be calling this from an EndState.  So call the PC function that performs cleanup, not the one that sets GI state.
	if (LocalPrimaryController != NULL)
	{
		LocalPrimaryController->HandleReturnToMainMenu();
	}
}

void ASoldierGameMode::DetermineMatchWinner()
{
	// nothing to do here
}

bool ASoldierGameMode::IsWinner(class ASoldierPlayerState* PlayerState) const
{
	return false;
}

void ASoldierGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	ASoldierGameState* const MyGameState = Cast<ASoldierGameState>(GameState);
	const bool bMatchIsOver = MyGameState && MyGameState->HasMatchEnded();
	if( bMatchIsOver )
	{
		ErrorMessage = TEXT("Match is over!");
	}
	else
	{
		// GameSession can be NULL if the match is over
		Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	}
}


void ASoldierGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// update spectator location for client
	ASoldierPlayerController* NewPC = Cast<ASoldierPlayerController>(NewPlayer);
	if (NewPC && NewPC->GetPawn() == NULL)
	{
		NewPC->ClientSetSpectatorCamera(NewPC->GetSpawnLocation(), NewPC->GetControlRotation());
	}

	// notify new player if match is already in progress
	if (NewPC && IsMatchInProgress())
	{
		NewPC->ClientGameStarted();
		NewPC->ClientStartOnlineGame();
	}
}

void ASoldierGameMode::Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, const UDamageType* DamageType)
{
	ASoldierPlayerState* KillerPlayerState = Killer ? Cast<ASoldierPlayerState>(Killer->PlayerState) : NULL;
	ASoldierPlayerState* VictimPlayerState = KilledPlayer ? Cast<ASoldierPlayerState>(KilledPlayer->PlayerState) : NULL;

	if (KillerPlayerState && KillerPlayerState != VictimPlayerState)
	{
		KillerPlayerState->ScoreKill(VictimPlayerState, KillScore);
		KillerPlayerState->InformAboutKill(KillerPlayerState, DamageType, VictimPlayerState);
	}

	if (VictimPlayerState)
	{
		VictimPlayerState->ScoreDeath(KillerPlayerState, DeathScore);
		VictimPlayerState->BroadcastDeath(KillerPlayerState, DamageType, VictimPlayerState);
	}
}

float ASoldierGameMode::ModifyDamage(float Damage, AActor* DamagedActor, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) const
{
	float ActualDamage = Damage;

	ASoldierCharacter* DamagedPawn = Cast<ASoldierCharacter>(DamagedActor);
	if (DamagedPawn && EventInstigator)
	{
		ASoldierPlayerState* DamagedPlayerState = Cast<ASoldierPlayerState>(DamagedPawn->GetPlayerState());
		ASoldierPlayerState* InstigatorPlayerState = Cast<ASoldierPlayerState>(EventInstigator->PlayerState);

		// disable friendly fire
		if (!CanDealDamage(InstigatorPlayerState, DamagedPlayerState))
		{
			ActualDamage = 0.0f;
		}

		// scale self instigated damage
		if (InstigatorPlayerState == DamagedPlayerState)
		{
			ActualDamage *= DamageSelfScale;
		}
	}

	return ActualDamage;
}

bool ASoldierGameMode::CanDealDamage(class ASoldierPlayerState* DamageInstigator, class ASoldierPlayerState* DamagedPlayer) const
{
	return true;
}

bool ASoldierGameMode::AllowCheats(APlayerController* P)
{
	return true;
}

bool ASoldierGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	return false;
}

UClass* ASoldierGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (InController->IsA<ASoldierAIController>())
	{
		return BotPawnClass;
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void ASoldierGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	ASoldierPlayerController* PC = Cast<ASoldierPlayerController>(NewPlayer);
	if (PC)
	{
		// Since initial weapon is equipped before the pawn is added to the replication graph, need to resend the notify so that it can be added as a dependent actor
		ASoldierCharacter* Character = Cast<ASoldierCharacter>(PC->GetCharacter());
		if (Character)
		{
			ASoldierCharacter::NotifyEquipWeapon.Broadcast(Character, Character->GetWeapon());
		}
		
		PC->ClientGameStarted();
	}
}

AActor* ASoldierGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	TArray<APlayerStart*> PreferredSpawns;
	TArray<APlayerStart*> FallbackSpawns;

	APlayerStart* BestStart = NULL;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* TestSpawn = *It;
		if (TestSpawn->IsA<APlayerStartPIE>())
		{
			// Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
			BestStart = TestSpawn;
			break;
		}
		else
		{
			if (IsSpawnpointAllowed(TestSpawn, Player))
			{
				if (IsSpawnpointPreferred(TestSpawn, Player))
				{
					PreferredSpawns.Add(TestSpawn);
				}
				else
				{
					FallbackSpawns.Add(TestSpawn);
				}
			}
		}
	}

	
	if (BestStart == NULL)
	{
		if (PreferredSpawns.Num() > 0)
		{
			BestStart = PreferredSpawns[FMath::RandHelper(PreferredSpawns.Num())];
		}
		else if (FallbackSpawns.Num() > 0)
		{
			BestStart = FallbackSpawns[FMath::RandHelper(FallbackSpawns.Num())];
		}
	}

	return BestStart ? BestStart : Super::ChoosePlayerStart_Implementation(Player);
}

bool ASoldierGameMode::IsSpawnpointAllowed(APlayerStart* SpawnPoint, AController* Player) const
{
	ASoldierTeamStart* SoldierSpawnPoint = Cast<ASoldierTeamStart>(SpawnPoint);
	if (SoldierSpawnPoint)
	{
		ASoldierAIController* AIController = Cast<ASoldierAIController>(Player);
		if (SoldierSpawnPoint->bNotForBots && AIController)
		{
			return false;
		}

		if (SoldierSpawnPoint->bNotForPlayers && AIController == NULL)
		{
			return false;
		}
		return true;
	}

	return false;
}

bool ASoldierGameMode::IsSpawnpointPreferred(APlayerStart* SpawnPoint, AController* Player) const
{
	ACharacter* MyPawn = Cast<ACharacter>((*DefaultPawnClass)->GetDefaultObject<ACharacter>());	
	ASoldierAIController* AIController = Cast<ASoldierAIController>(Player);
	if( AIController != nullptr )
	{
		MyPawn = Cast<ACharacter>(BotPawnClass->GetDefaultObject<ACharacter>());
	}
	
	if (MyPawn)
	{
		const FVector SpawnLocation = SpawnPoint->GetActorLocation();
		for (ACharacter* OtherPawn : TActorRange<ACharacter>(GetWorld()))
		{
			if (OtherPawn != MyPawn)
			{
				const float CombinedHeight = (MyPawn->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + OtherPawn->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()) * 2.0f;
				const float CombinedRadius = MyPawn->GetCapsuleComponent()->GetScaledCapsuleRadius() + OtherPawn->GetCapsuleComponent()->GetScaledCapsuleRadius();
				const FVector OtherLocation = OtherPawn->GetActorLocation();

				// check if player start overlaps this pawn
				if (FMath::Abs(SpawnLocation.Z - OtherLocation.Z) < CombinedHeight && (SpawnLocation - OtherLocation).Size2D() < CombinedRadius)
				{
					return false;
				}
			}
		}
	}
	else
	{
		return false;
	}
	
	return true;
}

void ASoldierGameMode::CreateBotControllers()
{
	UWorld* World = GetWorld();
	int32 ExistingBots = 0;
	for (FConstControllerIterator It = World->GetControllerIterator(); It; ++It)
	{		
		ASoldierAIController* AIC = Cast<ASoldierAIController>(*It);
		if (AIC)
		{
			++ExistingBots;
		}
	}

	// Create any necessary AIControllers.  Hold off on Pawn creation until pawns are actually necessary or need recreating.	
	int32 BotNum = ExistingBots;
	for (int32 i = 0; i < MaxBots - ExistingBots; ++i)
	{
		CreateBot(BotNum + i);
	}
}

ASoldierAIController* ASoldierGameMode::CreateBot(int32 BotNum)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = nullptr;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnInfo.OverrideLevel = nullptr;

	UWorld* World = GetWorld();
	ASoldierAIController* AIC = World->SpawnActor<ASoldierAIController>(SpawnInfo);
	InitBot(AIC, BotNum);

	return AIC;
}

void ASoldierGameMode::StartBots()
{
	// checking number of existing human player.
	UWorld* World = GetWorld();
	for (FConstControllerIterator It = World->GetControllerIterator(); It; ++It)
	{		
		ASoldierAIController* AIC = Cast<ASoldierAIController>(*It);
		if (AIC)
		{
			RestartPlayer(AIC);
		}
	}	
}

void ASoldierGameMode::InitBot(ASoldierAIController* AIController, int32 BotNum)
{	
	if (AIController)
	{
		if (AIController->PlayerState)
		{
			FString BotName = FString::Printf(TEXT("Bot %d"), BotNum);
			AIController->PlayerState->SetPlayerName(BotName);
		}		
	}
}

void ASoldierGameMode::RestartGame()
{
	// Hide the scoreboard too !
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		ASoldierPlayerController* PlayerController = Cast<ASoldierPlayerController>(*It);
		if (PlayerController != nullptr)
		{
			ASoldierHUD* SoldierHUD = Cast<ASoldierHUD>(PlayerController->GetHUD());
			if (SoldierHUD != nullptr)
			{
				// Passing true to bFocus here ensures that focus is returned to the game viewport.
				SoldierHUD->ShowScoreboard(false, true);
			}
		}
	}

	Super::RestartGame();
}


// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "RSAction.h"
#include "SSoldierScoreboardWidget.h"
#include "SoldierStyle.h"
#include "SoldierScoreboardWidgetStyle.h"
#include "SoldierUIHelpers.h"
#include "Online/SoldierPlayerState.h"

#define LOCTEXT_NAMESPACE "SoldierScoreboard"

// @todo: prevent interaction on PC for now (see OnFocusReceived for reasons)
#if PLATFORM_XBOXONE
#define INTERACTIVE_SCOREBOARD	1
#else
#define INTERACTIVE_SCOREBOARD	0
#endif

#define	NORM_PADDING	(FMargin(5))

void SSoldierScoreboardWidget::Construct(const FArguments& InArgs)
{
	ScoreboardStyle = &FSoldierStyle::Get().GetWidgetStyle<FSoldierScoreboardStyle>("DefaultSoldierScoreboardStyle");

	PCOwner = InArgs._PCOwner;
	ScoreboardTint = FLinearColor(0.0f,0.0f,0.0f,0.4f);
	ScoreBoxWidth = 140.0f;
	ScoreCountUpTime = 2.0f;

	ScoreboardStartTime = FPlatformTime::Seconds();
	MatchState = InArgs._MatchState.Get();

	UpdatePlayerStateMaps();
	
	Columns.Add(FColumnData(LOCTEXT("KillsColumn", "Kills"),
		ScoreboardStyle->KillStatColor,
		FOnGetPlayerStateAttribute::CreateSP(this, &SSoldierScoreboardWidget::GetAttributeValue_Kills)));

	Columns.Add(FColumnData(LOCTEXT("DeathsColumn", "Deaths"),
		ScoreboardStyle->DeathStatColor,
		FOnGetPlayerStateAttribute::CreateSP(this, &SSoldierScoreboardWidget::GetAttributeValue_Deaths)));

	Columns.Add(FColumnData(LOCTEXT("ScoreColumn", "Score"),
		ScoreboardStyle->ScoreStatColor,
		FOnGetPlayerStateAttribute::CreateSP(this, &SSoldierScoreboardWidget::GetAttributeValue_Score)));

	TSharedPtr<SHorizontalBox> HeaderCols;

	const TSharedRef<SVerticalBox> ScoreboardGrid = SNew(SVerticalBox)
	// HEADER ROW			
	+SVerticalBox::Slot() .AutoHeight()
	[
		//Padding in the header row (invisible) for speaker icon
		SAssignNew(HeaderCols, SHorizontalBox)
		+ SHorizontalBox::Slot().Padding(NORM_PADDING+FMargin(2,0,0,0)).AutoWidth()
		[
			SNew(SImage)
			.Image(FSoldierStyle::Get().GetBrush("SoldierGame.Speaker"))
			.Visibility(EVisibility::Hidden)
		]

		//Player Name autosized column
		+SHorizontalBox::Slot() .Padding(NORM_PADDING)
		[
			SNew(SBorder)
			.Padding(NORM_PADDING)
			.VAlign(VAlign_Center)	
			.HAlign(HAlign_Center)
			.BorderImage(&ScoreboardStyle->ItemBorderBrush)
			.BorderBackgroundColor(ScoreboardTint)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PlayerNameColumn", "Player Name"))
				.TextStyle(FSoldierStyle::Get(), "SoldierGame.DefaultScoreboard.Row.HeaderTextStyle")
			]
		]
	];

	for (uint8 ColIdx = 0; ColIdx < Columns.Num(); ColIdx++ )
	{
		//Header constant sized columns
		HeaderCols->AddSlot() .VAlign(VAlign_Center) .HAlign(HAlign_Center) .AutoWidth() .Padding(NORM_PADDING)
		[
			SNew(SBorder)
			.Padding(NORM_PADDING)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.BorderImage(&ScoreboardStyle->ItemBorderBrush)
			.BorderBackgroundColor(ScoreboardTint)
			[
				SNew(SBox)
				.WidthOverride(ScoreBoxWidth)
				.HAlign(HAlign_Center)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot() .AutoWidth() .VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(Columns[ColIdx].Name)
						.TextStyle(FSoldierStyle::Get(), "SoldierGame.DefaultScoreboard.Row.HeaderTextStyle")
						.ColorAndOpacity(Columns[ColIdx].Color)
					]
				]
			]
		];
	}

	ScoreboardGrid->AddSlot() .AutoHeight()
	[
		SAssignNew(ScoreboardData, SVerticalBox)
	];
	UpdateScoreboardGrid();

	SBorder::Construct(
		SBorder::FArguments()
		.BorderImage(&ScoreboardStyle->ItemBorderBrush)
		.BorderBackgroundColor(ScoreboardTint)
		[
			ScoreboardGrid
		]			
	);
}

void SSoldierScoreboardWidget::StoreTalkingPlayerData(const FUniqueNetId& PlayerId, bool bIsTalking)
{
	static TMap<FString, double> LastTimeSpoken;
	int32 FoundIndex = -1;
	const double IconTimeout = 0.1;
	double CurrentTime = FPlatformTime::Seconds();

	for (int32 i = 0; i < PlayersTalkingThisFrame.Num(); ++i)
	{
		if (PlayersTalkingThisFrame[i].Key.Get() == PlayerId)
		{
			FoundIndex = i;
		}
	}

	if (FoundIndex < 0)
	{
		FoundIndex = PlayersTalkingThisFrame.Emplace(PlayerId.AsShared(), false);
	}

	if (bIsTalking)
	{
		double* Value = LastTimeSpoken.Find(PlayerId.ToString());
		if (Value)
		{
			*Value = CurrentTime;
		}
		else
		{
			LastTimeSpoken.Add(PlayerId.ToString(), CurrentTime);
		}
	}

	if (LastTimeSpoken.FindRef(PlayerId.ToString()) > CurrentTime - IconTimeout)
	{
		PlayersTalkingThisFrame[FoundIndex].Value = true;
	}
	else
	{
		PlayersTalkingThisFrame[FoundIndex].Value = false;
	}
}

FText SSoldierScoreboardWidget::GetMatchRestartText() const
{
	if (PCOwner.IsValid() && (PCOwner->GetWorld() != NULL ))
	{
		ASoldierGameState* const GameState = PCOwner->GetWorld()->GetGameState<ASoldierGameState>();
		if (GameState)
		{
			if (GameState->RemainingTime > 0)
			{
				return FText::Format(LOCTEXT("MatchRestartTimeString", "New match begins in: {0}"), FText::AsNumber(GameState->RemainingTime));
			}
			else
			{
				return LOCTEXT("MatchRestartingString", "Starting new match...");
			}
		}
	}

	return FText::GetEmpty();
}

FText SSoldierScoreboardWidget::GetMatchOutcomeText() const
{
	FText OutcomeText = FText::GetEmpty();

	if (MatchState == ESoldierMatchState::Won)
	{
		OutcomeText = LOCTEXT("Winner", "YOU ARE THE WINNER!");
	} 
	else if (MatchState == ESoldierMatchState::Lost)
	{
		OutcomeText = LOCTEXT("Loser", "Match has finished");
	}

	return OutcomeText;
}

void SSoldierScoreboardWidget::UpdateScoreboardGrid()
{
	ScoreboardData->ClearChildren();
	for (uint8 TeamNum = 0; TeamNum < PlayerStateMaps.Num(); TeamNum++)
	{
		//Player rows from each team
		ScoreboardData->AddSlot() .AutoHeight()
			[
				MakePlayerRows(TeamNum)
			];
		//If we have more than one team, we are playing team based game mode, add totals
		if (PlayerStateMaps.Num() > 1 && PlayerStateMaps[TeamNum].Num() > 0)
		{
			// Horizontal Ruler
			ScoreboardData->AddSlot() .AutoHeight() .Padding(NORM_PADDING)
				[
					SNew(SBorder)
					.Padding(1)
					.BorderImage(&ScoreboardStyle->ItemBorderBrush)
				];
			ScoreboardData->AddSlot() .AutoHeight()
				[
					MakeTotalsRow(TeamNum)
				];
		}
	}

	if (MatchState > ESoldierMatchState::Playing)
	{
		ScoreboardData->AddSlot() .AutoHeight() .Padding(NORM_PADDING)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot() .HAlign(HAlign_Fill)
				[
					SNew(SBox)
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &SSoldierScoreboardWidget::GetMatchOutcomeText)
						.TextStyle(FSoldierStyle::Get(), "SoldierGame.MenuHeaderTextStyle")
					]
				]
			];

		ScoreboardData->AddSlot() .AutoHeight() .Padding(NORM_PADDING)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot() .HAlign(HAlign_Fill)
				[
					SNew(SBox)
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &SSoldierScoreboardWidget::GetMatchRestartText)
						.TextStyle(FSoldierStyle::Get(), "SoldierGame.MenuHeaderTextStyle")
					]
				]
			];

		ScoreboardData->AddSlot() .AutoHeight() .Padding(NORM_PADDING)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot() .HAlign(HAlign_Right)
				[
					SNew(SBox)
					.HAlign(HAlign_Right)
					[
						SNew(STextBlock)
						.Text(SoldierUIHelpers::Get().GetProfileOpenText())
						.TextStyle(FSoldierStyle::Get(), "SoldierGame.DefaultScoreboard.Row.StatTextStyle")
						.Visibility(this, &SSoldierScoreboardWidget::GetProfileUIVisibility)
					]
				]
			];
	}
}

void SSoldierScoreboardWidget::UpdatePlayerStateMaps()
{
	if (PCOwner.IsValid())
	{
		ASoldierGameState* const GameState = PCOwner->GetWorld()->GetGameState<ASoldierGameState>();
		if (GameState)
		{
			bool bRequiresWidgetUpdate = false;
			const int32 NumTeams = FMath::Max(GameState->NumTeams, 1);
			LastTeamPlayerCount.Reset();
			LastTeamPlayerCount.AddZeroed(PlayerStateMaps.Num());
			for (int32 i = 0; i < PlayerStateMaps.Num(); i++)
			{
				LastTeamPlayerCount[i] = PlayerStateMaps[i].Num();
			}

			PlayerStateMaps.Reset();
			PlayerStateMaps.AddZeroed(NumTeams);
		
			for (int32 i = 0; i < NumTeams; i++)
			{
				GameState->GetRankedMap(i, PlayerStateMaps[i]);

				if (LastTeamPlayerCount.Num() > 0 && PlayerStateMaps[i].Num() != LastTeamPlayerCount[i])
				{
					bRequiresWidgetUpdate = true;
				}
			}
			if (bRequiresWidgetUpdate)
			{
				UpdateScoreboardGrid();
			}
		}
	}

	UpdateSelectedPlayer();
}

void SSoldierScoreboardWidget::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	UpdatePlayerStateMaps();
}

bool SSoldierScoreboardWidget::SupportsKeyboardFocus() const
{
#if INTERACTIVE_SCOREBOARD
	if (MatchState > ESoldierMatchState::Playing)
	{
		return true;
	}
#endif
	return false;
}

FReply SSoldierScoreboardWidget::OnFocusReceived( const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent )
{
	// @todo: may not want to affect all controllers if split screen

	// @todo: not-pc: need to support mouse focus too (alt+tabbing, windowed, etc)
	// @todo: not-pc: after each round, the mouse is released but as soon as you click it's recaptured and input stops working
	return FReply::Handled().ReleaseMouseCapture().SetUserFocus(SharedThis(this), EFocusCause::SetDirectly, true);
}

FReply SSoldierScoreboardWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) 
{
	FReply Result = FReply::Unhandled();
	const FKey Key = InKeyEvent.GetKey();
	if (MatchState > ESoldierMatchState::Playing)
	{
		if (Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up)
		{
			OnSelectedPlayerPrev();
			Result = FReply::Handled();
		}
		else if (Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down)
		{
			OnSelectedPlayerNext();
			Result = FReply::Handled();
		}
		else if (Key == EKeys::Enter || Key == EKeys::Virtual_Accept)
		{
			ProfileUIOpened();
			Result = FReply::Handled();
		}
		else if ((Key == EKeys::Escape || Key == EKeys::Gamepad_Special_Right || Key == EKeys::Global_Play || Key == EKeys::Global_Menu) && !InKeyEvent.IsRepeat())
		{
			// Allow the user to pause from the scoreboard still
			if (ASoldierPlayerController* const PC = Cast<ASoldierPlayerController>(PCOwner.Get()))
			{
				PC->OnToggleInGameMenu();
			}
			Result = FReply::Handled();
		}
	}
	return Result;
}

void SSoldierScoreboardWidget::PlaySound(const FSlateSound& SoundToPlay) const
{
	if( PCOwner.IsValid() )
	{
		if( ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(PCOwner->Player) )
		{
			FSlateApplication::Get().PlaySound(SoundToPlay, LocalPlayer->GetControllerId());	
		}
	}
}

FReply SSoldierScoreboardWidget::OnMouseOverPlayer(const FGeometry& Geometry, const FPointerEvent& Event, const FTeamPlayer TeamPlayer)
{
#if INTERACTIVE_SCOREBOARD
	if( !(SelectedPlayer == TeamPlayer) )
	{
		SelectedPlayer = TeamPlayer;
		PlaySound(ScoreboardStyle->PlayerChangeSound);
	}
#endif
	return FReply::Handled();
}

void SSoldierScoreboardWidget::OnSelectedPlayerPrev()
{
	// Make sure we have a valid index to start with
	if( !SelectedPlayer.IsValid() && !SetSelectedPlayerUs())
	{
		return;
	}

	// We don't want to decrease the player Id, we want to find the current one in the list and then select the one before it
	int32 PrevPlayerId = -1;
	for (RankedPlayerMap::TConstIterator PlayerIt(PlayerStateMaps[SelectedPlayer.TeamNum]); PlayerIt; ++PlayerIt)	// This would be easier with a .ReverseIterator call
	{
		const int32 PlayerId = PlayerIt.Key();
		if( PlayerId == SelectedPlayer.PlayerId )
		{
			break;
		}
		PrevPlayerId = PlayerId;
	}

	if( PrevPlayerId == -1 )
	{
		// If the id is still invalid, that means the current selection was first in their team, try the previous team...
		SelectedPlayer.TeamNum--;
		if (!PlayerStateMaps.IsValidIndex(SelectedPlayer.TeamNum))
		{
			// If there isn't a previous team, move to the last team
			SelectedPlayer.TeamNum = PlayerStateMaps.Num() - 1;
			check(PlayerStateMaps.IsValidIndex(SelectedPlayer.TeamNum));
		}

		// We want the last player in the team
		for (RankedPlayerMap::TConstIterator PlayerIt(PlayerStateMaps[SelectedPlayer.TeamNum]); PlayerIt; ++PlayerIt)	// This would be easier with a .Last call
		{
			const int32 PlayerId = PlayerIt.Key();
			PrevPlayerId = PlayerId;
		}
	}

	check( SelectedPlayer.PlayerId != -1 );
	SelectedPlayer.PlayerId = PrevPlayerId;
	PlaySound(ScoreboardStyle->PlayerChangeSound);
}

void SSoldierScoreboardWidget::OnSelectedPlayerNext()
{
	// Make sure we have a valid index to start with
	if( !SelectedPlayer.IsValid() && !SetSelectedPlayerUs())
	{
		return;
	}

	// We don't want to increase the player Id, we want to find the current one in the list and then select the one after
	bool bNext = false;
	for (RankedPlayerMap::TConstIterator PlayerIt(PlayerStateMaps[SelectedPlayer.TeamNum]); PlayerIt; ++PlayerIt)
	{
		const int32 PlayerId = PlayerIt.Key();
		if( PlayerId == SelectedPlayer.PlayerId )
		{
			bNext = true;
		}
		else if( bNext == true )
		{
			SelectedPlayer.PlayerId = PlayerId;
			PlaySound(ScoreboardStyle->PlayerChangeSound);

			bNext = false;
			break;
		}
	}

	if( bNext == true )
	{
		// If next is still true, our current selection was last in their team, try the next team...
		SelectedPlayer.TeamNum++;
		if (!PlayerStateMaps.IsValidIndex(SelectedPlayer.TeamNum))
		{
			// If there isn't a next team, move to the first team
			SelectedPlayer.TeamNum = 0;
			check(PlayerStateMaps.IsValidIndex(SelectedPlayer.TeamNum));
		}

		SelectedPlayer.PlayerId = 0;
		PlaySound(ScoreboardStyle->PlayerChangeSound);
	}
}

void SSoldierScoreboardWidget::ResetSelectedPlayer()
{
	SelectedPlayer = FTeamPlayer();
}

void SSoldierScoreboardWidget::UpdateSelectedPlayer()
{
	// Make sure the selected player is still valid...
	if( SelectedPlayer.IsValid() )
	{
		const ASoldierPlayerState* PlayerState = GetSortedPlayerState(SelectedPlayer);
		if( !PlayerState )
		{
			// Player is no longer valid, reset (note: reset implies 'us' in IsSelectedPlayer and IsPlayerSelectedAndValid)
			ResetSelectedPlayer();
		}
	}
}

bool SSoldierScoreboardWidget::SetSelectedPlayerUs()
{
	ResetSelectedPlayer();

	// Set the owner player to be the default focused one
	if( APlayerController* const PC = PCOwner.Get() )
	{
		for (uint8 TeamNum = 0; TeamNum < PlayerStateMaps.Num(); TeamNum++)
		{
			for (RankedPlayerMap::TConstIterator PlayerIt(PlayerStateMaps[TeamNum]); PlayerIt; ++PlayerIt)
			{
				const TWeakObjectPtr<ASoldierPlayerState> PlayerState = PlayerIt.Value();
				if( PlayerState.IsValid() && PC->PlayerState && PC->PlayerState == PlayerState.Get() )
				{
					SelectedPlayer = FTeamPlayer(TeamNum, PlayerIt.Key());
					return true;
				}
			}
		}
	}
	return false;
}

bool SSoldierScoreboardWidget::IsSelectedPlayer(const FTeamPlayer& TeamPlayer) const
{
	if( !SelectedPlayer.IsValid() )
	{
		// If not explicitly set, test to see if the owner player was passed.
		if( IsOwnerPlayer(TeamPlayer) )
		{
			return true;
		}
	}
	else if( SelectedPlayer == TeamPlayer )
	{
		return true;
	}
	return false;
}

bool SSoldierScoreboardWidget::IsPlayerSelectedAndValid() const
{
#if INTERACTIVE_SCOREBOARD
	if( !SelectedPlayer.IsValid() )
	{
		// Nothing is selected, default to the player
		if( PCOwner.IsValid() && PCOwner->PlayerState )
		{
			const TSharedPtr<const FUniqueNetId> OwnerNetId = PCOwner->PlayerState->UniqueId.GetUniqueNetId();
			return OwnerNetId.IsValid();
		}
	}
	else if( const ASoldierPlayerState* PlayerState = GetSortedPlayerState(SelectedPlayer) ) 
	{
		const TSharedPtr<const FUniqueNetId>& PlayerId = PlayerState->UniqueId.GetUniqueNetId();
		return PlayerId.IsValid();
	}
#endif
	return false;
}

EVisibility SSoldierScoreboardWidget::GetProfileUIVisibility() const
{
	return IsPlayerSelectedAndValid() ? EVisibility::Visible : EVisibility::Hidden;
}

bool SSoldierScoreboardWidget::ProfileUIOpened() const
{
	if( IsPlayerSelectedAndValid() )
	{
		check( PCOwner.IsValid() && PCOwner->PlayerState );
		const TSharedPtr<const FUniqueNetId>& OwnerNetId = PCOwner->PlayerState->UniqueId.GetUniqueNetId();
		check( OwnerNetId.IsValid() );

		const TSharedPtr<const FUniqueNetId>& PlayerId = ( !SelectedPlayer.IsValid() ? OwnerNetId : GetSortedPlayerState(SelectedPlayer)->UniqueId.GetUniqueNetId() );
		check( PlayerId.IsValid() );
		return SoldierUIHelpers::Get().ProfileOpenedUI(PCOwner->GetWorld(), *OwnerNetId.Get(), *PlayerId.Get(), NULL);
	}
	return false;
}

EVisibility SSoldierScoreboardWidget::PlayerPresenceToItemVisibility(const FTeamPlayer TeamPlayer) const
{
	return GetSortedPlayerState(TeamPlayer) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SSoldierScoreboardWidget::SpeakerIconVisibility(const FTeamPlayer TeamPlayer) const
{
	ASoldierPlayerState* PlayerState = GetSortedPlayerState(TeamPlayer);
	if (PlayerState)
	{
		for (int32 i = 0; i < PlayersTalkingThisFrame.Num(); ++i)
		{
			if (PlayerState->UniqueId == PlayersTalkingThisFrame[i].Key && PlayersTalkingThisFrame[i].Value)
			{
				return EVisibility::Visible;
			}
		}
	}
	return EVisibility::Hidden;
}

FSlateColor SSoldierScoreboardWidget::GetScoreboardBorderColor(const FTeamPlayer TeamPlayer) const
{
	const bool bIsSelected = IsSelectedPlayer(TeamPlayer);
	const int32 RedTeam = 0;
	const float BaseValue = bIsSelected == true ? 0.15f : 0.0f;
	const float AlphaValue = bIsSelected == true ? 1.0f : 0.3f;
	float RedValue = TeamPlayer.TeamNum == RedTeam ? 0.25f : 0.0f;
	float BlueValue = TeamPlayer.TeamNum != RedTeam ? 0.25f : 0.0f;
	return FLinearColor(BaseValue + RedValue, BaseValue, BaseValue + BlueValue, AlphaValue);
}

FText SSoldierScoreboardWidget::GetPlayerName(const FTeamPlayer TeamPlayer) const
{
	const ASoldierPlayerState* PlayerState = GetSortedPlayerState(TeamPlayer);
	if (PlayerState)
	{
		return FText::FromString(PlayerState->GetShortPlayerName());
	}

	return FText::GetEmpty();
}

bool SSoldierScoreboardWidget::ShouldPlayerBeDisplayed(const FTeamPlayer TeamPlayer) const
{
	const ASoldierPlayerState* PlayerState = GetSortedPlayerState(TeamPlayer);
	
	return PlayerState != nullptr && !PlayerState->bOnlySpectator;
}

FSlateColor SSoldierScoreboardWidget::GetPlayerColor(const FTeamPlayer TeamPlayer) const
{
	// If this is the owner players row, tint the text color to show ourselves more clearly
	if( IsOwnerPlayer(TeamPlayer) )
	{
		return FSlateColor(FLinearColor::Yellow);
	}

	const FTextBlockStyle& TextStyle = FSoldierStyle::Get().GetWidgetStyle<FTextBlockStyle>("SoldierGame.DefaultScoreboard.Row.StatTextStyle");
	return TextStyle.ColorAndOpacity;
}

FSlateColor SSoldierScoreboardWidget::GetColumnColor(const FTeamPlayer TeamPlayer, uint8 ColIdx) const
{
	// If this is the owner players row, tint the text color to show ourselves more clearly
	if( IsOwnerPlayer(TeamPlayer) )
	{
		return FSlateColor(FLinearColor::Yellow);
	}

	check(Columns.IsValidIndex(ColIdx));
	return Columns[ColIdx].Color;
}

bool SSoldierScoreboardWidget::IsOwnerPlayer(const FTeamPlayer& TeamPlayer) const
{
	return ( PCOwner.IsValid() && PCOwner->PlayerState && PCOwner->PlayerState == GetSortedPlayerState(TeamPlayer) );
}

FText SSoldierScoreboardWidget::GetStat(FOnGetPlayerStateAttribute Getter, const FTeamPlayer TeamPlayer) const
{
	int32 StatTotal = 0;
	if (TeamPlayer.PlayerId != SpecialPlayerIndex::All)
	{
		ASoldierPlayerState* PlayerState = GetSortedPlayerState(TeamPlayer);
		if (PlayerState)
		{
			StatTotal = Getter.Execute(PlayerState);
		}
	} 
	else
	{
		for (RankedPlayerMap::TConstIterator PlayerIt(PlayerStateMaps[TeamPlayer.TeamNum]); PlayerIt; ++PlayerIt)
		{
			ASoldierPlayerState* PlayerState = PlayerIt.Value().Get();
			if (PlayerState)
			{
				StatTotal += Getter.Execute(PlayerState);
			}
		}
	}

	return FText::AsNumber(LerpForCountup(StatTotal));
}

int32 SSoldierScoreboardWidget::LerpForCountup(int32 ScoreValue) const
{
	if (MatchState > ESoldierMatchState::Playing)
	{
		const float LerpAmount = FMath::Min<float>(1.0f, (FPlatformTime::Seconds() - ScoreboardStartTime) / ScoreCountUpTime);
		return FMath::Lerp(0, ScoreValue, LerpAmount);
	}
	else
	{
		return ScoreValue;
	}
}

TSharedRef<SWidget> SSoldierScoreboardWidget::MakeTotalsRow(uint8 TeamNum) const
{
	TSharedPtr<SHorizontalBox> TotalsRow;

	SAssignNew(TotalsRow, SHorizontalBox)
	+SHorizontalBox::Slot() .Padding(NORM_PADDING)
	[
		SNew(SBorder)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(NORM_PADDING)
		.BorderImage(&ScoreboardStyle->ItemBorderBrush)
		.BorderBackgroundColor(ScoreboardTint)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ScoreTotal", "Team Score"))
			.TextStyle(FSoldierStyle::Get(), "SoldierGame.DefaultScoreboard.Row.HeaderTextStyle")
		]
	];

	//Leave two columns empty
	for (uint8 i = 0; i < 2; i++)
	{
		TotalsRow->AddSlot() .Padding(NORM_PADDING) .AutoWidth() .HAlign(HAlign_Center) .VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.Padding(NORM_PADDING)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.BorderImage(FStyleDefaults::GetNoBrush())
			.BorderBackgroundColor(ScoreboardTint)
			[
				SNew(SBox)
				.WidthOverride(ScoreBoxWidth)
				.HAlign(HAlign_Center)
			]
		];
	}
	//Total team score / captures in CTF mode
	TotalsRow->AddSlot() .Padding(NORM_PADDING) .AutoWidth() .HAlign(HAlign_Center) .VAlign(VAlign_Center)
	[
		SNew(SBorder)
		.Padding(NORM_PADDING)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.BorderImage(&ScoreboardStyle->ItemBorderBrush)
		.BorderBackgroundColor(ScoreboardTint)
		[
			SNew(SBox)
			.WidthOverride(ScoreBoxWidth)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SSoldierScoreboardWidget::GetStat, Columns.Last().AttributeGetter, FTeamPlayer(TeamNum, SpecialPlayerIndex::All))
				.TextStyle(FSoldierStyle::Get(), "SoldierGame.DefaultScoreboard.Row.HeaderTextStyle")
			]
		]
	];

	return TotalsRow.ToSharedRef();
}

TSharedRef<SWidget> SSoldierScoreboardWidget::MakePlayerRows(uint8 TeamNum) const
{
	TSharedRef<SVerticalBox> PlayerRows = SNew(SVerticalBox);

	for (int32 PlayerIndex=0; PlayerIndex < PlayerStateMaps[TeamNum].Num(); PlayerIndex++ )
	{
		FTeamPlayer TeamPlayer(TeamNum, PlayerIndex);

		if (ShouldPlayerBeDisplayed(TeamPlayer))
		{
			PlayerRows->AddSlot().AutoHeight()
				[
					MakePlayerRow(TeamPlayer)
				];
		}
	}

	return PlayerRows;
}

TSharedRef<SWidget> SSoldierScoreboardWidget::MakePlayerRow(const FTeamPlayer& TeamPlayer) const
{
	// Make the padding here slightly smaller than NORM_PADDING, to fit in more players
	const FMargin Pad = FMargin(5,1);

	TSharedPtr<SHorizontalBox> PlayerRow;
	//Speaker Icon display
	SAssignNew(PlayerRow, SHorizontalBox)
	+ SHorizontalBox::Slot().Padding(Pad+FMargin(2,0,0,0)).AutoWidth()
	[
		SNew(SImage)
		.Image(FSoldierStyle::Get().GetBrush("SoldierGame.Speaker"))
		.Visibility(this, &SSoldierScoreboardWidget::SpeakerIconVisibility, TeamPlayer)
	];

	//first autosized row with player name
	PlayerRow->AddSlot() .Padding(Pad)
	[
		SNew(SBorder)
		.Padding(Pad)
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		.Visibility(this, &SSoldierScoreboardWidget::PlayerPresenceToItemVisibility, TeamPlayer)
		.OnMouseMove(const_cast<SSoldierScoreboardWidget*>(this), &SSoldierScoreboardWidget::OnMouseOverPlayer, TeamPlayer)
		.BorderBackgroundColor(const_cast<SSoldierScoreboardWidget*>(this), &SSoldierScoreboardWidget::GetScoreboardBorderColor, TeamPlayer)
		.BorderImage(&ScoreboardStyle->ItemBorderBrush)
		[
			SNew(STextBlock)
			.Text(this, &SSoldierScoreboardWidget::GetPlayerName, TeamPlayer)
			.TextStyle(FSoldierStyle::Get(), "SoldierGame.DefaultScoreboard.Row.StatTextStyle")
			.ColorAndOpacity(this, &SSoldierScoreboardWidget::GetPlayerColor, TeamPlayer)
		]
	];
	//attributes rows (kills, deaths, score/captures)
	for (uint8 ColIdx = 0; ColIdx < Columns.Num(); ColIdx++)
	{
		PlayerRow->AddSlot()
		.Padding(Pad) .AutoWidth() .HAlign(HAlign_Center) .VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.Padding(Pad)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.Visibility(this, &SSoldierScoreboardWidget::PlayerPresenceToItemVisibility, TeamPlayer)
			.OnMouseMove(const_cast<SSoldierScoreboardWidget*>(this), &SSoldierScoreboardWidget::OnMouseOverPlayer, TeamPlayer)
			.BorderBackgroundColor(this, &SSoldierScoreboardWidget::GetScoreboardBorderColor, TeamPlayer)
			.BorderImage(&ScoreboardStyle->ItemBorderBrush)
			[
				SNew(SBox)
				.WidthOverride(ScoreBoxWidth)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(this, &SSoldierScoreboardWidget::GetStat, Columns[ColIdx].AttributeGetter, TeamPlayer)
					.TextStyle(FSoldierStyle::Get(), "SoldierGame.DefaultScoreboard.Row.StatTextStyle")
					.ColorAndOpacity(this, &SSoldierScoreboardWidget::GetColumnColor, TeamPlayer, ColIdx)
				]
			]
		];
	}
	return PlayerRow.ToSharedRef();
}

ASoldierPlayerState* SSoldierScoreboardWidget::GetSortedPlayerState(const FTeamPlayer& TeamPlayer) const
{
	if (PlayerStateMaps.IsValidIndex(TeamPlayer.TeamNum) && PlayerStateMaps[TeamPlayer.TeamNum].Contains(TeamPlayer.PlayerId))
	{
		return PlayerStateMaps[TeamPlayer.TeamNum].FindRef(TeamPlayer.PlayerId).Get();
	}
	
	return NULL;
}

int32 SSoldierScoreboardWidget::GetAttributeValue_Kills(ASoldierPlayerState* PlayerState) const
{
	return PlayerState->GetKills();
}

int32 SSoldierScoreboardWidget::GetAttributeValue_Deaths(ASoldierPlayerState* PlayerState) const
{
	return PlayerState->GetDeaths();
}

int32 SSoldierScoreboardWidget::GetAttributeValue_Score(ASoldierPlayerState* PlayerState) const
{
	return FMath::TruncToInt(PlayerState->Score);
}

#undef LOCTEXT_NAMESPACE

// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Styling/SlateWidgetStyleContainerBase.h"
#include "SoldierScoreboardWidgetStyle.generated.h"

/**
 * Represents the appearance of an SSoldierScoreboardWidget
 */
USTRUCT()
struct FSoldierScoreboardStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FSoldierScoreboardStyle();
	virtual ~FSoldierScoreboardStyle();

	// FSlateWidgetStyle
	virtual void GetResources(TArray<const FSlateBrush*>& OutBrushes) const override;
	static const FName TypeName;
	virtual const FName GetTypeName() const override { return TypeName; };
	static const FSoldierScoreboardStyle& GetDefault();

	/**
	 * The brush used for the item border
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush ItemBorderBrush;
	FSoldierScoreboardStyle& SetItemBorderBrush(const FSlateBrush& InItemBorderBrush) { ItemBorderBrush = InItemBorderBrush; return *this; }

	/**
	 * The color used for the kill stat
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor KillStatColor;
	FSoldierScoreboardStyle& SetKillStatColor(const FSlateColor& InKillStatColor) { KillStatColor = InKillStatColor; return *this; }

	/**
	 * The color used for the death stat
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor DeathStatColor;
	FSoldierScoreboardStyle& SetDeathStatColor(const FSlateColor& InDeathStatColor) { DeathStatColor = InDeathStatColor; return *this; }

	/**
	 * The color used for the score stat
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor ScoreStatColor;
	FSoldierScoreboardStyle& SetScoreStatColor(const FSlateColor& InScoreStatColor) { ScoreStatColor = InScoreStatColor; return *this; }

	/**
	 * The sound that should play when the highlighted player changes in the scoreboard
	 */	
	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound PlayerChangeSound;
	FSoldierScoreboardStyle& SetPlayerChangeSound(const FSlateSound& InPlayerChangeSound) { PlayerChangeSound = InPlayerChangeSound; return *this; }
};


/**
 */
UCLASS(hidecategories=Object, MinimalAPI)
class USoldierScoreboardWidgetStyle : public USlateWidgetStyleContainerBase
{
	GENERATED_UCLASS_BODY()

public:
	/** The actual data describing the scoreboard's appearance. */
	UPROPERTY(Category=Appearance, EditAnywhere, meta=(ShowOnlyInnerProperties))
	FSoldierScoreboardStyle ScoreboardStyle;

	virtual const struct FSlateWidgetStyle* const GetStyle() const override
	{
		return static_cast< const struct FSlateWidgetStyle* >( &ScoreboardStyle );
	}
};

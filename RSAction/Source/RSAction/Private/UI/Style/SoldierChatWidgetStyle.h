// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateWidgetStyleContainerBase.h"
#include "SoldierChatWidgetStyle.generated.h"

/**
 * Represents the appearance of an SChatWidget
 */
USTRUCT()
struct FSoldierChatStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FSoldierChatStyle();
	virtual ~FSoldierChatStyle();

	// FSlateWidgetStyle
	virtual void GetResources(TArray<const FSlateBrush*>& OutBrushes) const override;
	static const FName TypeName;
	virtual const FName GetTypeName() const override { return TypeName; };
	static const FSoldierChatStyle& GetDefault();

	/**
	 * The style used for entering chat text
	 */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FEditableTextBoxStyle TextEntryStyle;
	FSoldierChatStyle& SetChatEntryStyle(const FEditableTextBoxStyle& InTextEntryStyle) { TextEntryStyle = InTextEntryStyle; return *this; }

	/**
	 * The brush used for the chat backing
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush BackingBrush;
	FSoldierChatStyle& SetBackingBrush(const FSlateBrush& InBackingBrush) { BackingBrush = InBackingBrush; return *this; }

	/**
	 * The color used for the chat box border
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor BoxBorderColor;
	FSoldierChatStyle& SetBoxBorderColor(const FSlateColor& InBoxBorderColor) { BoxBorderColor = InBoxBorderColor; return *this; }

	/**
	 * The color used for the chat box text
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor TextColor;
	FSoldierChatStyle& SetTextColor(const FSlateColor& InTextColor) { TextColor = InTextColor; return *this; }

	/**
	 * The sound that should play when receiving a chat message
	 */	
	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound RxMessgeSound;
	FSoldierChatStyle& SetRxMessgeSound(const FSlateSound& InRxMessgeSound) { RxMessgeSound = InRxMessgeSound; return *this; }

	/**
	 * The sound that should play when sending a chat message
	 */	
	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound TxMessgeSound;
	FSoldierChatStyle& SetTxMessgeSound(const FSlateSound& InTxMessgeSound) { TxMessgeSound = InTxMessgeSound; return *this; }
};


/**
 */
UCLASS(hidecategories=Object, MinimalAPI)
class USoldierChatWidgetStyle : public USlateWidgetStyleContainerBase
{
	GENERATED_UCLASS_BODY()

public:
	/** The actual data describing the chat appearance. */
	UPROPERTY(Category=Appearance, EditAnywhere, meta=(ShowOnlyInnerProperties))
	FSoldierChatStyle ChatStyle;

	virtual const struct FSlateWidgetStyle* const GetStyle() const override
	{
		return static_cast< const struct FSlateWidgetStyle* >( &ChatStyle );
	}
};

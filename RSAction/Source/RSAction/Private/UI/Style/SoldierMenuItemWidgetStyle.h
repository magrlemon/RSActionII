// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateWidgetStyleContainerBase.h"
#include "SoldierMenuItemWidgetStyle.generated.h"

/**
 * Represents the appearance of an FSoldierMenuItem
 */
USTRUCT()
struct FSoldierMenuItemStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FSoldierMenuItemStyle();
	virtual ~FSoldierMenuItemStyle();

	// FSlateWidgetStyle
	virtual void GetResources(TArray<const FSlateBrush*>& OutBrushes) const override;
	static const FName TypeName;
	virtual const FName GetTypeName() const override { return TypeName; };
	static const FSoldierMenuItemStyle& GetDefault();

	/**
	 * The brush used for the item background
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush BackgroundBrush;
	FSoldierMenuItemStyle& SetBackgroundBrush(const FSlateBrush& InBackgroundBrush) { BackgroundBrush = InBackgroundBrush; return *this; }

	/**
	 * The image used for the left arrow
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush LeftArrowImage;
	FSoldierMenuItemStyle& SetLeftArrowImage(const FSlateBrush& InLeftArrowImage) { LeftArrowImage = InLeftArrowImage; return *this; }

	/**
	 * The image used for the right arrow
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush RightArrowImage;
	FSoldierMenuItemStyle& SetRightArrowImage(const FSlateBrush& InRightArrowImage) { RightArrowImage = InRightArrowImage; return *this; }
};


/**
 */
UCLASS(hidecategories=Object, MinimalAPI)
class USoldierMenuItemWidgetStyle : public USlateWidgetStyleContainerBase
{
	GENERATED_UCLASS_BODY()

public:
	/** The actual data describing the menu's appearance. */
	UPROPERTY(Category=Appearance, EditAnywhere, meta=(ShowOnlyInnerProperties))
	FSoldierMenuItemStyle MenuItemStyle;

	virtual const struct FSlateWidgetStyle* const GetStyle() const override
	{
		return static_cast< const struct FSlateWidgetStyle* >( &MenuItemStyle );
	}
};

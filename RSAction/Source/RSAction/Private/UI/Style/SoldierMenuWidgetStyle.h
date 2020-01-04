// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Styling/SlateWidgetStyleContainerBase.h"
#include "SoldierMenuWidgetStyle.generated.h"

/**
 * Represents the appearance of an SSoldierMenuWidget
 */
USTRUCT()
struct FSoldierMenuStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FSoldierMenuStyle();
	virtual ~FSoldierMenuStyle();

	// FSlateWidgetStyle
	virtual void GetResources(TArray<const FSlateBrush*>& OutBrushes) const override;
	static const FName TypeName;
	virtual const FName GetTypeName() const override { return TypeName; };
	static const FSoldierMenuStyle& GetDefault();

	/**
	 * The brush used for the header background
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush HeaderBackgroundBrush;
	FSoldierMenuStyle& SetHeaderBackgroundBrush(const FSlateBrush& InHeaderBackgroundBrush) { HeaderBackgroundBrush = InHeaderBackgroundBrush; return *this; }

	/**
	 * The brush used for the left side of the menu
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush LeftBackgroundBrush;
	FSoldierMenuStyle& SetLeftBackgroundBrush(const FSlateBrush& InLeftBackgroundBrush) { LeftBackgroundBrush = InLeftBackgroundBrush; return *this; }

	/**
	 * The brush used for the right side of the menu
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush RightBackgroundBrush;
	FSoldierMenuStyle& SetRightBackgroundBrush(const FSlateBrush& InRightBackgroundBrush) { RightBackgroundBrush = InRightBackgroundBrush; return *this; }

	/**
	 * The sound that should play when entering a sub-menu
	 */	
	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound MenuEnterSound;
	FSoldierMenuStyle& SetMenuEnterSound(const FSlateSound& InMenuEnterSound) { MenuEnterSound = InMenuEnterSound; return *this; }

	/**
	 * The sound that should play when leaving a sub-menu
	 */	
	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound MenuBackSound;
	FSoldierMenuStyle& SetMenuBackSound(const FSlateSound& InMenuBackSound) { MenuBackSound = InMenuBackSound; return *this; }

	/**
	 * The sound that should play when changing an option
	 */	
	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound OptionChangeSound;
	FSoldierMenuStyle& SetOptionChangeSound(const FSlateSound& InOptionChangeSound) { OptionChangeSound = InOptionChangeSound; return *this; }

	/**
	 * The sound that should play when changing the selected menu item
	 */	
	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound MenuItemChangeSound;
	FSoldierMenuStyle& SetMenuItemChangeSound(const FSlateSound& InMenuItemChangeSound) { MenuItemChangeSound = InMenuItemChangeSound; return *this; }
};


/**
 */
UCLASS(hidecategories=Object, MinimalAPI)
class USoldierMenuWidgetStyle : public USlateWidgetStyleContainerBase
{
	GENERATED_UCLASS_BODY()

public:
	/** The actual data describing the menu's appearance. */
	UPROPERTY(Category=Appearance, EditAnywhere, meta=(ShowOnlyInnerProperties))
	FSoldierMenuStyle MenuStyle;

	virtual const struct FSlateWidgetStyle* const GetStyle() const override
	{
		return static_cast< const struct FSlateWidgetStyle* >( &MenuStyle );
	}
};

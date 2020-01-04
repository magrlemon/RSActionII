// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SoldierGame.h"
#include "SoldierMenuItemWidgetStyle.h"

FSoldierMenuItemStyle::FSoldierMenuItemStyle()
{
}

FSoldierMenuItemStyle::~FSoldierMenuItemStyle()
{
}

const FName FSoldierMenuItemStyle::TypeName(TEXT("FSoldierMenuItemStyle"));

const FSoldierMenuItemStyle& FSoldierMenuItemStyle::GetDefault()
{
	static FSoldierMenuItemStyle Default;
	return Default;
}

void FSoldierMenuItemStyle::GetResources(TArray<const FSlateBrush*>& OutBrushes) const
{
	OutBrushes.Add(&BackgroundBrush);
	OutBrushes.Add(&LeftArrowImage);
	OutBrushes.Add(&RightArrowImage);
}


USoldierMenuItemWidgetStyle::USoldierMenuItemWidgetStyle( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	
}

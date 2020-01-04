// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SoldierGame.h"
#include "SoldierMenuWidgetStyle.h"

FSoldierMenuStyle::FSoldierMenuStyle()
{
}

FSoldierMenuStyle::~FSoldierMenuStyle()
{
}

const FName FSoldierMenuStyle::TypeName(TEXT("FSoldierMenuStyle"));

const FSoldierMenuStyle& FSoldierMenuStyle::GetDefault()
{
	static FSoldierMenuStyle Default;
	return Default;
}

void FSoldierMenuStyle::GetResources(TArray<const FSlateBrush*>& OutBrushes) const
{
	OutBrushes.Add(&HeaderBackgroundBrush);
	OutBrushes.Add(&LeftBackgroundBrush);
	OutBrushes.Add(&RightBackgroundBrush);
}


USoldierMenuWidgetStyle::USoldierMenuWidgetStyle( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	
}

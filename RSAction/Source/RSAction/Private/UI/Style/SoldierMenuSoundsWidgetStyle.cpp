// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "RSAction.h"
#include "SoldierMenuSoundsWidgetStyle.h"

FSoldierMenuSoundsStyle::FSoldierMenuSoundsStyle()
{
}

FSoldierMenuSoundsStyle::~FSoldierMenuSoundsStyle()
{
}

const FName FSoldierMenuSoundsStyle::TypeName(TEXT("FSoldierMenuSoundsStyle"));

const FSoldierMenuSoundsStyle& FSoldierMenuSoundsStyle::GetDefault()
{
	static FSoldierMenuSoundsStyle Default;
	return Default;
}

void FSoldierMenuSoundsStyle::GetResources(TArray<const FSlateBrush*>& OutBrushes) const
{
}


USoldierMenuSoundsWidgetStyle::USoldierMenuSoundsWidgetStyle( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	
}

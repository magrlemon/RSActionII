// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SoldierGame.h"
#include "SoldierOptionsWidgetStyle.h"

FSoldierOptionsStyle::FSoldierOptionsStyle()
{
}

FSoldierOptionsStyle::~FSoldierOptionsStyle()
{
}

const FName FSoldierOptionsStyle::TypeName(TEXT("FSoldierOptionsStyle"));

const FSoldierOptionsStyle& FSoldierOptionsStyle::GetDefault()
{
	static FSoldierOptionsStyle Default;
	return Default;
}

void FSoldierOptionsStyle::GetResources(TArray<const FSlateBrush*>& OutBrushes) const
{
}


USoldierOptionsWidgetStyle::USoldierOptionsWidgetStyle( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	
}

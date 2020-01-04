// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "RSAction.h"
#include "SoldierScoreboardWidgetStyle.h"

FSoldierScoreboardStyle::FSoldierScoreboardStyle()
{
}

FSoldierScoreboardStyle::~FSoldierScoreboardStyle()
{
}

const FName FSoldierScoreboardStyle::TypeName(TEXT("FSoldierScoreboardStyle"));

const FSoldierScoreboardStyle& FSoldierScoreboardStyle::GetDefault()
{
	static FSoldierScoreboardStyle Default;
	return Default;
}

void FSoldierScoreboardStyle::GetResources(TArray<const FSlateBrush*>& OutBrushes) const
{
	OutBrushes.Add(&ItemBorderBrush);
}


USoldierScoreboardWidgetStyle::USoldierScoreboardWidgetStyle( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	
}

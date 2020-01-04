// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SoldierGame.h"
#include "SoldierChatWidgetStyle.h"

FSoldierChatStyle::FSoldierChatStyle()
{
}

FSoldierChatStyle::~FSoldierChatStyle()
{
}

const FName FSoldierChatStyle::TypeName(TEXT("FSoldierChatStyle"));

const FSoldierChatStyle& FSoldierChatStyle::GetDefault()
{
	static FSoldierChatStyle Default;
	return Default;
}

void FSoldierChatStyle::GetResources(TArray<const FSlateBrush*>& OutBrushes) const
{
	TextEntryStyle.GetResources(OutBrushes);

	OutBrushes.Add(&BackingBrush);
}


USoldierChatWidgetStyle::USoldierChatWidgetStyle( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	
}

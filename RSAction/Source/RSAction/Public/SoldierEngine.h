// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SoldierEngine.generated.h"

UCLASS()
class RSACTION_API USoldierEngine : public UGameEngine
{
	GENERATED_UCLASS_BODY()

	/* Hook up specific callbacks */
	virtual void Init(IEngineLoop* InEngineLoop);

public:

	/**
	 * 	All regular engine handling, plus update SoldierKing state appropriately.
	 */
	virtual void HandleNetworkFailure(UWorld *World, UNetDriver *NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString) override;
};


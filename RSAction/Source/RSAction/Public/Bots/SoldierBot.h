// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SoldierCharacter.h"
#include "SoldierBot.generated.h"

UCLASS()
class ASoldierBot : public ASoldierCharacter
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Behavior)
	class UBehaviorTree* BotBehavior;

	virtual bool IsFirstPerson() const override;

	virtual void FaceRotation(FRotator NewRotation, float DeltaTime = 0.f) override;
};
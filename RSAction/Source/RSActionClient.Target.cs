// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class RSActionClientTarget : TargetRules
{
	public RSActionClientTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Client;
		bUsesSteam = true;		

        ExtraModuleNames.Add("RSAction");
    }
}

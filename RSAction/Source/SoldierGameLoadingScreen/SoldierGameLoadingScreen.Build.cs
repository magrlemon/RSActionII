// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

// This module must be loaded "PreLoadingScreen" in the .uproject file, otherwise it will not hook in time!

public class SoldierGameLoadingScreen : ModuleRules
{
    public SoldierGameLoadingScreen( ReadOnlyTargetRules Target) : base(Target)
	{
		PrivatePCHHeaderFile = "Public/SoldierGameLoadingScreen.h";

		PCHUsage = PCHUsageMode.UseSharedPCHs;

        PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
                "CoreUObject",
				"MoviePlayer",
				"Slate",
				"SlateCore",
				"InputCore"
			}
		);
	}
}

// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ECSystemPlugin : ModuleRules
{
	public ECSystemPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
                //"ECSystemPlugin\\Private\\Event",
                //"ECSystemPlugin\\Private\\Memory",
                //"ECSystemPlugin\\Private\\util",
                //"ECSystemPlugin\\Private\\Memory\\Allocator"
            }
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
                 "ECSystemPlugin\\Public\\Event",
                "ECSystemPlugin\\Public\\Memory",
                "ECSystemPlugin\\Public\\util",
                "ECSystemPlugin\\Public\\Memory\\Allocator"
            }
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}

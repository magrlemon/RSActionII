// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RSAction : ModuleRules {
    public RSAction( ReadOnlyTargetRules Target ) : base( Target ) {
        PrivatePCHHeaderFile = "Public/RSAction.h";


        PublicIncludePaths.AddRange(
                  new string[] {
                "RSAction/Public",
                "RSAction/Public/Bots",
                "RSAction/Public/Effects",
                "RSAction/Public/Online",
                "RSAction/Public/Pickups",
                "RSAction/Public/Sound",
                "RSAction/Public/Player",
                "RSAction/Public/Weapons",
                "RSAction/Public/UI"
                  }
              );

        PrivateIncludePaths.AddRange(
            new string[] {
                "RSAction/Private",
                "RSAction/Private/UI",
                "RSAction/Private/UI/Menu",
                "RSAction/Private/UI/Style",
                "RSAction/Private/UI/Widgets",
            }
        );

        PublicDependencyModuleNames.AddRange( new string[] {
                "Core",
                "CoreUObject",
                "Engine",
                "OnlineSubsystem",
                "OnlineSubsystemUtils",
                "AssetRegistry",
                "NavigationSystem",
                "AIModule",
                "GameplayTasks",
                "Gauntlet",
                "HeadMountedDisplay",
                "ECSystemPlugin"
             }
        );

        PrivateDependencyModuleNames.AddRange(
          new string[] {
                "InputCore",
                "Slate",
                "SlateCore",
                "SoldierGameLoadingScreen",
                "Json",
                "ApplicationCore",
                "ReplicationGraph",
                "PakFile",
                "RHI"
          }
      );


        DynamicallyLoadedModuleNames.AddRange(
            new string[] {
                "OnlineSubsystemNull",
                "NetworkReplayStreaming",
                "NullNetworkReplayStreaming",
                "HttpNetworkReplayStreaming",
                "LocalFileNetworkReplayStreaming"
            }
        );

        PrivateIncludePathModuleNames.AddRange(
            new string[] {
                "NetworkReplayStreaming"
            }
        );

        if (Target.bBuildDeveloperTools || (Target.Configuration != UnrealTargetConfiguration.Shipping && Target.Configuration != UnrealTargetConfiguration.Test)) {
            PrivateDependencyModuleNames.Add( "GameplayDebugger" );
            PublicDefinitions.Add( "WITH_GAMEPLAY_DEBUGGER=1" );
        }
        else {
            PublicDefinitions.Add( "WITH_GAMEPLAY_DEBUGGER=0" );
        }
    }
}

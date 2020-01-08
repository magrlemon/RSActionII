// Copyright 2016 Dotex Games. All Rights Reserved.

using System.IO;

namespace UnrealBuildTool.Rules
{
    public class DcxVehicleEditor : ModuleRules
    {
        public DcxVehicleEditor(ReadOnlyTargetRules Target)
            : base(Target)
        {
            PublicDependencyModuleNames.AddRange(
                new string[] {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "AnimGraph",
                    "AnimGraphRuntime",
                    "BlueprintGraph",
                    "UnrealEd",
                    "DcxVehicle"
                }
            );
        }
    }
}
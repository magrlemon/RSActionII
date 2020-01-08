// Copyright 2016 Dotex Games. All Rights Reserved.

using System.IO;

namespace UnrealBuildTool.Rules
{
	public class DcxVehicle : ModuleRules
	{
		public DcxVehicle(ReadOnlyTargetRules Target)
            : base(Target)
		{
			PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "AnimGraphRuntime",
                    "PhysX",
                    "PhysXVehicleLib",
                    "APEX",
                    "PhysicsCore"
                }
            );
		}
    }
}
// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TG : ModuleRules
{
	public TG(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "NavigationSystem", "ProceduralMeshComponent", "Foliage", "PhysicsCore" });
	}
}

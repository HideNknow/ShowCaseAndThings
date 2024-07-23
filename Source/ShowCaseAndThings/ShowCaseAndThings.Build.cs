// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ShowCaseAndThings : ModuleRules
{
	public ShowCaseAndThings(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(new string[] {  });
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "HeadMountedDisplay" , "ProceduralMeshComponent" });
	}
}

// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Night_of_the_Wolves : ModuleRules
{
	public Night_of_the_Wolves(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "AnimGraphRuntime", "HairStrandsCore", "Renderer" });

		PublicIncludePaths.AddRange(new string[] {
			"Night_of_the_Wolves",
			"Night_of_the_Wolves/Variant_Platforming",
			"Night_of_the_Wolves/Variant_Platforming/Animation",
			"Night_of_the_Wolves/Variant_Combat",
			"Night_of_the_Wolves/Variant_Combat/AI",
			"Night_of_the_Wolves/Variant_Combat/Animation",
			"Night_of_the_Wolves/Variant_Combat/Gameplay",
			"Night_of_the_Wolves/Variant_Combat/Interfaces",
			"Night_of_the_Wolves/Variant_Combat/UI",
			"Night_of_the_Wolves/Variant_SideScrolling",
			"Night_of_the_Wolves/Variant_SideScrolling/AI",
			"Night_of_the_Wolves/Variant_SideScrolling/Gameplay",
			"Night_of_the_Wolves/Variant_SideScrolling/Interfaces",
			"Night_of_the_Wolves/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}

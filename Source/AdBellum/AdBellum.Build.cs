// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class AdBellum : ModuleRules
{
	public AdBellum(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
       // PrivatePCHHeaderFile = "Private/WindowsMixedRealityPrecompiled.h";

        PublicDependencyModuleNames.AddRange(new string[] { "Core","PhysicsCore", "CoreUObject", "Engine", "InputCore", "AdvancedSessions", "VRExpansionPlugin", 
			"AIModule", "ALSV4_CPP", "EnhancedInput", "NavigationSystem", "Chaos","ChaosVehicles", "DragonIKPlugin", "EasyBallistics",
			"Niagara", "NetCore", "Networking", "VehicleSystemPlugin", "NPC_Optimizator","RecoilAnimation"});

		if (Target.Type == TargetRules.TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {"RecentAssetsMenu", "LiveBlueprintDebugger", "Pjc", "AutoSizeComments"});
		}

		PrivateDependencyModuleNames.AddRange(new string[] { "GameplayTasks", "OnlineSubsystem", "OnlineSubsystemNull", "OnlineSubsystemSteam" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}

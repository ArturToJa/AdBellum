// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class AdBellum : ModuleRules
{
	public AdBellum(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
       // PrivatePCHHeaderFile = "Private/WindowsMixedRealityPrecompiled.h";

        PublicDependencyModuleNames.AddRange(new string[] { "Core","PhysicsCore", "CoreUObject", "Engine", "InputCore","AdvancedSessions",

            "AIModule", "ALSV4_CPP", "EnhancedInput", "NavigationSystem", "Chaos","ChaosVehicles", "DragonIKPlugin", "EasyBallistics",
			"Niagara", "NetCore", "Networking", "VehicleSystemPlugin", "RecoilAnimation","WorldDirectorPRO", "UMG", "UBSplineStructureCreator", "NPC_Optimizator"});

		if (Target.Type == TargetRules.TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {"Pjc", "AutoSizeComments"});
            //removed "LiveBlueprintDebugger" "RecentAssetsMenu"
        }

        PrivateDependencyModuleNames.AddRange(new string[] { "GameplayTasks", "OnlineSubsystem", "OnlineSubsystemNull", "OnlineSubsystemSteam", "Slate", "SlateCore" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true

		PrivateIncludePaths.AddRange(new string[] {	ModuleDirectory	});
	}
}

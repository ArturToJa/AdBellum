// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class AdBellumEditorTarget : TargetRules
{
	public AdBellumEditorTarget(TargetInfo Target) : base(Target)
	{
        DefaultBuildSettings = BuildSettingsVersion.Latest;

        Type = TargetType.Editor;
        ExtraModuleNames.AddRange(new string[] { "AdBellum" });
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		//bOverrideBuildEnvironment = true;
		//bUseUnityBuild = false;
		//bUsePCHFiles = false;
	}

	//
	// TargetRules interface.
	//

/*	public override void SetupBinaries(
		TargetInfo Target,
		ref List<UEBuildBinaryConfiguration> OutBuildBinaryConfigurations,
		ref List<string> OutExtraModuleNames
		)
	{
		OutExtraModuleNames.AddRange( new string[] { "AdBellum" } );
	}*/
}

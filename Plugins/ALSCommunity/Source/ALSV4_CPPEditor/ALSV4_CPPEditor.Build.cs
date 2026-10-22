using UnrealBuildTool;

// Editor-only tools for the ALS anim blueprint (never part of packaged builds).
public class ALSV4_CPPEditor : ModuleRules
{
	public ALSV4_CPPEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"Core", "CoreUObject", "Engine", "UnrealEd", "BlueprintGraph", "AnimGraph", "AnimGraphRuntime",
			"ALSV4_CPP"
		});
	}
}

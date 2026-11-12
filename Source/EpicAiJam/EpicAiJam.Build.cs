using UnrealBuildTool;

public class EpicAiJam : ModuleRules
{
	public EpicAiJam(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "AIModule", "UMG", "Slate", "SlateCore", "Niagara", "CinematicCamera", "DeveloperSettings" });

		PublicIncludePaths.Add(ModuleDirectory);
	}
}

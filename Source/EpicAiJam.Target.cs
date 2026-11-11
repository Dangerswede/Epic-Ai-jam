using UnrealBuildTool;

public class EpicAiJamTarget : TargetRules
{
	public EpicAiJamTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("EpicAiJam");
	}
}

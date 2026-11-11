using UnrealBuildTool;

public class EpicAiJamEditorTarget : TargetRules
{
	public EpicAiJamEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("EpicAiJam");
	}
}

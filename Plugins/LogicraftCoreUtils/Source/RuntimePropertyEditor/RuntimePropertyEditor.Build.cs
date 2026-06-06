using System.IO;
using UnrealBuildTool;

public class RuntimePropertyEditor : ModuleRules
{
    public RuntimePropertyEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Public", "RuntimePropertyEditor"));
        
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "Slate",
                "SlateCore",
                "DeveloperSettings"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "CoreUtils",
                "InputCore"
            }
        );
    }
}
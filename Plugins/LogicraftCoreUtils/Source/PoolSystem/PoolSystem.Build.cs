using System.IO;
using UnrealBuildTool;

public class PoolSystem : ModuleRules
{
    public PoolSystem(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        
        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Public", "PoolSystem"));

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", 
                "CoreUtils"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore"
            }
        );
    }
}
using System;
using System.Diagnostics;
using System.IO;
using UnrealBuildTool;

public class CoreUtilsMeta : ModuleRules
{
    public CoreUtilsMeta(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        
        PublicIncludePaths.AddRange(
            new string[]
            {
                ModuleDirectory + "/Public"
            });
        
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", 
                "CoreUObject", 
                "Engine"
            }
        );
    }
}
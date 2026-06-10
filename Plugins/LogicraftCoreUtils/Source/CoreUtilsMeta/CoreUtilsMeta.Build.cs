using UnrealBuildTool;

public class CoreUtilsMeta : ModuleRules
{
    public CoreUtilsMeta(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.Add(ModuleDirectory + "/Public");
        
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
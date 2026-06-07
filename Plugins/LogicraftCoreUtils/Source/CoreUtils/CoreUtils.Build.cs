using UnrealBuildTool;

public class CoreUtils : ModuleRules
{
    public CoreUtils(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", 
                "CoreUtilsMeta",
                "GameplayTags"
            }
        );
    
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
            }
        );

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange([ "UnrealEd" ]);
        }
    }
}
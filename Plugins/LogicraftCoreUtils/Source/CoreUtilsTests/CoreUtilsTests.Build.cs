using UnrealBuildTool;

public class CoreUtilsTests : ModuleRules
{
    public CoreUtilsTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "CoreUtils",
                "PoolSystem",
                "SaveSystem",
                "GameplayTags",
                "UnrealEd"
            }
        );
    }
}
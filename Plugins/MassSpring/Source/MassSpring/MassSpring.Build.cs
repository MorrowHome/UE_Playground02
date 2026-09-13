using UnrealBuildTool;

public class MassSpring : ModuleRules
{
    public MassSpring(ReadOnlyTargetRules target)
        : base(target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(["Core", "CoreUObject", "Engine"]);
        PrivateDependencyModuleNames.AddRange(["Projects", "RenderCore", "RHI"]);
    }
}

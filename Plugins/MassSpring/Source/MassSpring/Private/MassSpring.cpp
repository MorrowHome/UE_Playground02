#include "MassSpring.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

void FMassSpringModule::StartupModule()
{
    const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("MassSpring"));
    check(Plugin.IsValid());
    const FString ShaderDir = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
    AddShaderSourceDirectoryMapping(TEXT("/Plugin/MassSpring"), ShaderDir);
    UE_LOG(LogTemp, Display, TEXT("MassSpring module loaded. Shaders: %s"), *ShaderDir);
}

void FMassSpringModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FMassSpringModule, MassSpring)

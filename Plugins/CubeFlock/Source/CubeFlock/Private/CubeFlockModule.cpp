#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

class FCubeFlockModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("CubeFlock"));
        check(Plugin.IsValid());
        AddShaderSourceDirectoryMapping(TEXT("/Plugin/CubeFlock"), FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders")));
    }
};
IMPLEMENT_MODULE(FCubeFlockModule, CubeFlock)


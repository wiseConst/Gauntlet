#include <Gauntlet.h>
#include <Gauntlet/Core/Entrypoint.h>

#include "EditorLayer.h"

namespace Gauntlet
{

class GauntletEditor final : public Application
{
  public:
    GauntletEditor(const ApplicationSpecification& applicationSpec) : Application(applicationSpec) { PushLayer(new EditorLayer()); }

    ~GauntletEditor() {}
};

Scoped<Application> CreateApplication(const CommandLineArguments& args)
{
    ApplicationSpecification appSpec = {};
    appSpec.AppName                  = "Gauntlet Editor";
    appSpec.WindowLogoPath           = "../Resources/Logo/Gauntlet.png";
    appSpec.GraphicsAPI              = RendererAPI::EAPI::Vulkan;
    appSpec.bUseCustomTitleBar       = false;
    appSpec.CmdArgs                  = args;

    return MakeScoped<GauntletEditor>(appSpec);
}

}  // namespace Gauntlet
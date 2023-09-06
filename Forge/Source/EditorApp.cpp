#include <Gauntlet.h>
#include <Gauntlet/Core/Entrypoint.h>

#include "EditorLayer.h"

namespace Gauntlet
{

class GauntletEditor final : public Application
{
  public:
    GauntletEditor(const ApplicationSpecification& InApplicationSpec) : Application(InApplicationSpec) { PushLayer(new EditorLayer()); }

    ~GauntletEditor() {}
};

Scoped<Application> CreateApplication()
{
    ApplicationSpecification appSpec = {};
    appSpec.AppName                  = "Gauntlet Editor";

    const auto WindowLogoPath  = std::string("../Resources/Logo/Gauntlet.png");
    appSpec.WindowLogoPath     = WindowLogoPath.data();
    appSpec.GraphicsAPI        = RendererAPI::EAPI::Vulkan;
    appSpec.bUseCustomTitleBar = false;

    return MakeScoped<GauntletEditor>(appSpec);
}

}  // namespace Gauntlet
#include <Gauntlet.h>
#include <Gauntlet/Core/Entrypoint.h>

#include "EditorLayer.h"

class GauntletEditor final : public Gauntlet::Application
{
  public:
    GauntletEditor(const Gauntlet::ApplicationSpecification& InApplicationSpec) : Gauntlet::Application(InApplicationSpec)
    {
        PushLayer(new EditorLayer());
    }

    ~GauntletEditor() {}
};

Gauntlet::Scoped<Gauntlet::Application> Gauntlet::CreateApplication()
{
    Gauntlet::ApplicationSpecification AppSpec = {};
    AppSpec.AppName                            = "Gauntlet Editor";

    const auto WindowLogoPath  = std::string(ASSETS_PATH) + std::string("Logo/Gauntlet.png");
    AppSpec.WindowLogoPath     = WindowLogoPath.data();
    AppSpec.GraphicsAPI        = Gauntlet::RendererAPI::EAPI::Vulkan;
    AppSpec.bUseCustomTitleBar = false;

    return Gauntlet::MakeScoped<GauntletEditor>(AppSpec);
}

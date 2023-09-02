#include <Gauntlet.h>
#include <Gauntlet/Core/Entrypoint.h>

#include "RuntimeLayer.h"

class GauntletRuntime final : public Gauntlet::Application
{
  public:
    GauntletRuntime(const Gauntlet::ApplicationSpecification& InApplicationSpec) : Gauntlet::Application(InApplicationSpec)
    {
        PushLayer(new RuntimeLayer());
    }

    ~GauntletRuntime() {}
};

Gauntlet::Scoped<Gauntlet::Application> Gauntlet::CreateApplication()
{
    Gauntlet::ApplicationSpecification AppSpec = {};
    AppSpec.AppName                            = "Gauntlet Editor";

    const auto WindowLogoPath  = std::string("../Resources/Logo/Gauntlet.png");
    AppSpec.WindowLogoPath     = WindowLogoPath.data();
    AppSpec.GraphicsAPI        = Gauntlet::RendererAPI::EAPI::Vulkan;
    AppSpec.bUseCustomTitleBar = false;

    return Gauntlet::MakeScoped<GauntletRuntime>(AppSpec);
}

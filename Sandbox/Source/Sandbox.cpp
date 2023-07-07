#include <Eclipse.h>
#include <Eclipse/Core/Entrypoint.h>

#include "GravityExplorerLayer.h"

class Sandbox final : public Eclipse::Application
{
  public:
    Sandbox(const Eclipse::ApplicationSpecification& InApplicationSpec) : Eclipse::Application(InApplicationSpec)
    {
        PushLayer(new GravityExplorer());
    }

    ~Sandbox() {}
};

Eclipse::Scoped<Eclipse::Application> Eclipse::CreateApplication()
{
    Eclipse::ApplicationSpecification AppSpec = {};
    AppSpec.AppName = "Sandbox";
    AppSpec.WindowLogoPath = "Resources/Logo/Eclipse.jpg";
    AppSpec.GraphicsAPI = Eclipse::RendererAPI::EAPI::Vulkan;

    return Eclipse::MakeScoped<Sandbox>(AppSpec);
}

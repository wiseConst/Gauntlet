#include <Eclipse.h>
#include <Eclipse/Core/Entrypoint.h>

class Sandbox final : public Eclipse::Application
{
  public:
    Sandbox(const Eclipse::ApplicationSpecification& InApplicationSpec) : Eclipse::Application(InApplicationSpec) {}
    ~Sandbox() {}
};

Eclipse::Scoped<Eclipse::Application> Eclipse::CreateApplication()
{
    Eclipse::ApplicationSpecification AppSpec = {};
    AppSpec.AppName = "Sandbox";
    AppSpec.WindowLogoPath = "Resources/Logo/Eclipse.jpg";
    AppSpec.Width = 1024;
    AppSpec.Height = 768;
    AppSpec.GraphicsAPI = Eclipse::RendererAPI::EAPI::Vulkan;

    return Eclipse::MakeScoped<Sandbox>(AppSpec);
}

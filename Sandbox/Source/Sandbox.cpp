#include <Eclipse.h>
#include <Eclipse/Core/Entrypoint.h>

// #include "Pong2D/PongLayer.h"
// #include "1HourGame/GameLayer.h"

#include "Sandbox2DLayer.h"

class Sandbox final : public Eclipse::Application
{
  public:
    Sandbox(const Eclipse::ApplicationSpecification& InApplicationSpec) : Eclipse::Application(InApplicationSpec)
    {
        // PushLayer(new PongLayer());
        // PushLayer(new GameLayer());
        PushLayer(new Sandbox2DLayer());
    }

    ~Sandbox() {}
};

Eclipse::Scoped<Eclipse::Application> Eclipse::CreateApplication()
{
    Eclipse::ApplicationSpecification AppSpec = {};
    AppSpec.AppName                           = "Eclipse";

    const auto WindowLogoPath = std::string(ASSETS_PATH) + std::string("Logo/Eclipse.jpg");
    AppSpec.WindowLogoPath    = WindowLogoPath.data();
    AppSpec.GraphicsAPI       = Eclipse::RendererAPI::EAPI::Vulkan;

    return Eclipse::MakeScoped<Sandbox>(AppSpec);
}

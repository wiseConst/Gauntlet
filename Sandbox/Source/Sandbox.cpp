#include <Gauntlet.h>
#include <Gauntlet/Core/Entrypoint.h>

// #include "Pong2D/PongLayer.h"
// #include "1HourGame/GameLayer.h"

#include "Sandbox2DLayer.h"

class Sandbox final : public Gauntlet::Application
{
  public:
    Sandbox(const Gauntlet::ApplicationSpecification& InApplicationSpec) : Gauntlet::Application(InApplicationSpec)
    {
        // PushLayer(new PongLayer());
        // PushLayer(new GameLayer());
        PushLayer(new Sandbox2DLayer());
    }

    ~Sandbox() {}
};

Gauntlet::Scoped<Gauntlet::Application> Gauntlet::CreateApplication()
{
    Gauntlet::ApplicationSpecification AppSpec = {};
    AppSpec.AppName                            = "Gauntlet";

    const auto WindowLogoPath = std::string(ASSETS_PATH) + std::string("Logo/Gauntlet.jpg");
    AppSpec.WindowLogoPath    = WindowLogoPath.data();
    AppSpec.GraphicsAPI       = Gauntlet::RendererAPI::EAPI::Vulkan;

    return Gauntlet::MakeScoped<Sandbox>(AppSpec);
}

#include <Eclipse.h>
#include <Eclipse/Core/Entrypoint.h>

class TestLayer final : public Eclipse::Layer
{
  public:
    TestLayer() : Eclipse::Layer("TestLayer") {}
    ~TestLayer() = default;

    void OnUpdate(const float DeltaTime) final override {}

    void OnAttach() final override {}
    void OnDetach() final override {}

  private:
};

class Sandbox final : public Eclipse::Application
{
  public:
    Sandbox(const Eclipse::ApplicationSpecification& InApplicationSpec) : Eclipse::Application(InApplicationSpec)
    {
        PushLayer(new TestLayer());
    }

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

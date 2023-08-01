#include <Eclipse.h>
#include <Eclipse/Core/Entrypoint.h>

#include "EditorLayer.h"

class EclipseEditor final : public Eclipse::Application
{
  public:
    EclipseEditor(const Eclipse::ApplicationSpecification& InApplicationSpec) : Eclipse::Application(InApplicationSpec)
    {
        PushLayer(new EditorLayer());
    }

    ~EclipseEditor() {}
};

Eclipse::Scoped<Eclipse::Application> Eclipse::CreateApplication()
{
    Eclipse::ApplicationSpecification AppSpec = {};
    AppSpec.AppName                           = "Unreal Editor";

    const auto WindowLogoPath = std::string(ASSETS_PATH) + std::string("Logo/Eclipse.jpg");
    AppSpec.WindowLogoPath    = WindowLogoPath.data();
    AppSpec.GraphicsAPI       = Eclipse::RendererAPI::EAPI::Vulkan;

    return Eclipse::MakeScoped<EclipseEditor>(AppSpec);
}

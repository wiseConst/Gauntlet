#include "EclipsePCH.h"
#include "Renderer.h"

// #include "Platform/Vulkan/VulkanRenderer.h"

namespace Eclipse
{
Renderer* Renderer::s_Renderer = nullptr;

void Renderer::Init(RendererAPI::EAPI GraphicsAPI)
{

    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan:
        {
            // s_Renderer = new VulkanRenderer();
            return;
        }
        case RendererAPI::EAPI::None:
        {
            LOG_ERROR("RendererAPI::EAPI::None!");
        }
    }

    ELS_ASSERT(false, "Unknown RendererAPI!");
}

void Renderer::Shutdown()
{
    s_Renderer->Destroy();

    delete s_Renderer;
    s_Renderer = nullptr;
}

}  // namespace Eclipse
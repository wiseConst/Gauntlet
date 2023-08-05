#include "EclipsePCH.h"
#include "Renderer.h"

#include "Eclipse/Platform/Vulkan/VulkanRenderer.h"

namespace Eclipse
{
Renderer* Renderer::s_Renderer = nullptr;
Renderer::RendererStats Renderer::s_RendererStats;

void Renderer::Init()
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan:
        {
            s_Renderer = new VulkanRenderer();
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
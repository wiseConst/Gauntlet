#include "GauntletPCH.h"
#include "Renderer.h"

#include "Gauntlet/Platform/Vulkan/VulkanRenderer.h"

namespace Gauntlet
{
Renderer* Renderer::s_Renderer = nullptr;
std::mutex Renderer::s_ResourceAccessMutex;

Renderer::RendererStats Renderer::s_RendererStats;
Renderer::RendererSettings Renderer::s_RendererSettings;

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

    GNT_ASSERT(false, "Unknown RendererAPI!");
}

void Renderer::Shutdown()
{
    s_Renderer->Destroy();

    delete s_Renderer;
    s_Renderer = nullptr;
}

}  // namespace Gauntlet
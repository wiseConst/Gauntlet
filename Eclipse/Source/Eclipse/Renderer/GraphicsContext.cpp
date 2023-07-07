#include "EclipsePCH.h"
#include "GraphicsContext.h"

#include "Platform/Vulkan/VulkanContext.h"

namespace Eclipse
{
GraphicsContext* GraphicsContext::s_Context = nullptr;

GraphicsContext* GraphicsContext::Create(Scoped<Window>& InWindow)
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan:
        {
            LOG_INFO("RHI: Vulkan!");
            return new VulkanContext(InWindow);
        }
        case RendererAPI::EAPI::None:
        {
            LOG_ERROR("RendererAPI::EAPI::None!");
            return nullptr;
        }
    }

    ELS_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

}  // namespace Eclipse
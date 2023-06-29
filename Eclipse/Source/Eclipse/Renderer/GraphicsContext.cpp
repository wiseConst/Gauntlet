#include "EclipsePCH.h"
#include "GraphicsContext.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "RendererAPI.h"

namespace Eclipse
{
// Should not be [delete], it's just the pointer of "Application.h" m_Context.
GraphicsContext* GraphicsContext::s_Context = nullptr;

GraphicsContext* GraphicsContext::Create(Scoped<Window>& InWindow)
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan:
        {
            LOG_INFO("Using Vulkan Graphics API!");
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
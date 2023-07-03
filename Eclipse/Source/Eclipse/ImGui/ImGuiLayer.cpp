#include "EclipsePCH.h"
#include "ImGuiLayer.h"

#include "Eclipse/Renderer/RendererAPI.h"
#include "Platform/Vulkan/VulkanImGuiLayer.h"

namespace Eclipse
{
ImGuiLayer* ImGuiLayer::Create()
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan:
        {
            return new VulkanImGuiLayer();
        }
        case RendererAPI::EAPI::None:
        {
            return nullptr;
        }
    }

    ELS_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

}  // namespace Eclipse
#include "GauntletPCH.h"
#include "ImGuiLayer.h"

#include "Gauntlet/Renderer/RendererAPI.h"
#include "Gauntlet/Platform/Vulkan/VulkanImGuiLayer.h"

namespace Gauntlet
{
Scoped<ImGuiLayer> ImGuiLayer::Create()
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan:
        {
            return MakeScoped<VulkanImGuiLayer>();
        }
        case RendererAPI::EAPI::None:
        {
            return nullptr;
        }
    }

    GNT_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

}  // namespace Gauntlet
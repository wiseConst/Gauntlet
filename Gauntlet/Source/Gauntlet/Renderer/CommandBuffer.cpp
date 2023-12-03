#include "GauntletPCH.h"
#include "CommandBuffer.h"

#include "Gauntlet/Platform/Vulkan/VulkanCommandBuffer.h"

namespace Gauntlet
{
Ref<CommandBuffer> CommandBuffer::Create(ECommandBufferType type, ECommandBufferLevel level)
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan:
        {
            return MakeRef<VulkanCommandBuffer>(type, level);
        }
        case RendererAPI::EAPI::None:
        {
            LOG_ERROR("RendererAPI::EAPI::None!");
            GNT_ASSERT(false, "Unknown RendererAPI!");
            break;
        }
    }

    return nullptr;
}
}  // namespace Gauntlet
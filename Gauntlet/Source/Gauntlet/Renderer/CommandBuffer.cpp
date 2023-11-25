#include "GauntletPCH.h"
#include "CommandBuffer.h"

#include "Gauntlet/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Gauntlet/Platform/Vulkan/VulkanCommandPool.h"

namespace Gauntlet
{
Ref<CommandBuffer> CommandBuffer::Create(ECommandBufferType type)
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan:
        {
            return MakeRef<VulkanCommandBuffer>(type, true);
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
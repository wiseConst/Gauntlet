#pragma once

#include "Eclipse/Core/Core.h"
#include <volk/volk.h>

#include "VulkanCommandBuffer.h"

/*
    You can allocate as many VkCommandBuffer as you want from a given pool,
    but you can only record commands from one thread at a time.
    If you want multithreaded command recording, you need more VkCommandPool objects.
*/

namespace Eclipse
{

enum class ECommandPoolUsage : uint8_t
{
    COMMAND_POOL_DEFAULT_USAGE = 0,  // Has flag RESET_COMMAND_BUFFER
    COMMAND_POOL_TRANSFER_USAGE      // Has flags: RESET_COMMAND_BUFFER && TRANSIENT
};

struct CommandPoolSpecification
{
  public:
    CommandPoolSpecification() : QueueFamilyIndex(UINT32_MAX), CommandPoolUsage(ECommandPoolUsage::COMMAND_POOL_DEFAULT_USAGE) {}
    CommandPoolSpecification(const CommandPoolSpecification&) = default;

    static VkCommandPoolCreateFlags ConvertCommandPoolUsageFlagsToVulkan(ECommandPoolUsage InCommandPoolUsage)
    {
        /* Command buffer flags:
         *   1) VK_COMMAND_POOL_CREATE_TRANSIENT_BIT:
         *   Specifies that command buffers allocated from the pool will be short-lived,
         *   meaning that they will be reset or freed in a relatively short timeframe.
         *   This flag may be used by the implementation to control memory allocation behavior within the pool.
         *   2) VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT:
         *   Allows any command buffer allocated from a pool to be individually reset to the initial state;
         *   either by calling vkResetCommandBuffer, or via the implicit reset when calling vkBeginCommandBuffer.
         *   If this flag is not set on a pool, then vkResetCommandBuffer must not be called for any command buffer allocated from that
         * pool.
         */

        switch (InCommandPoolUsage)
        {
            case ECommandPoolUsage::COMMAND_POOL_DEFAULT_USAGE: return VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            case ECommandPoolUsage::COMMAND_POOL_TRANSFER_USAGE:
                return VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        }

        ELS_ASSERT(false, "Unknown ECommandPoolUsage!");
        return VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    }

    ECommandPoolUsage CommandPoolUsage;
    uint32_t QueueFamilyIndex;
};

class VulkanCommandPool final : private Uncopyable, private Unmovable
{
  public:
    VulkanCommandPool(const CommandPoolSpecification& InCommandPoolSpecification);
    VulkanCommandPool() = delete;
    ~VulkanCommandPool() = default;

    FORCEINLINE const auto& GetCommandBuffer(const uint32_t Index = 0) const { return m_CommandBuffers[Index]; }
    FORCEINLINE auto& GetCommandBuffer(const uint32_t Index = 0) { return m_CommandBuffers[Index]; }

    FORCEINLINE const auto& Get() const { return m_CommandPool; }
    FORCEINLINE auto& Get() { return m_CommandPool; }

    void Destroy();

  private:
    VkCommandPool m_CommandPool;
    CommandPoolSpecification m_CommandPoolSpecification;

    std::vector<VulkanCommandBuffer> m_CommandBuffers;

    void CreateCommandPool();
    void AllocateCommandBuffers();
};
}  // namespace Eclipse
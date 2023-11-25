#pragma once

#include "Gauntlet/Core/Core.h"
#include <volk/volk.h>

#include "VulkanCommandBuffer.h"

namespace Gauntlet
{

enum class ECommandPoolUsage : uint8_t
{
    COMMAND_POOL_USAGE_GRAPHICS = 0,  // Has flag RESET_COMMAND_BUFFER
    COMMAND_POOL_USAGE_TRANSFER,      // Has flags: RESET_COMMAND_BUFFER && TRANSIENT
    COMMAND_POOL_USAGE_COMPUTE        // Has flag: RESET_COMMAND_BUFFER
};

struct CommandPoolSpecification
{
  public:
    CommandPoolSpecification()                                = default;
    CommandPoolSpecification(const CommandPoolSpecification&) = default;

    static VkCommandPoolCreateFlags ConvertCommandPoolUsageFlagsToVulkan(ECommandPoolUsage commandPoolUsage)
    {
        switch (commandPoolUsage)
        {
            case ECommandPoolUsage::COMMAND_POOL_USAGE_COMPUTE:
            case ECommandPoolUsage::COMMAND_POOL_USAGE_GRAPHICS: return VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            case ECommandPoolUsage::COMMAND_POOL_USAGE_TRANSFER:
                return VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        }

        GNT_ASSERT(false, "Unknown ECommandPoolUsage!");
        return VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    }

    ECommandPoolUsage CommandPoolUsage = ECommandPoolUsage::COMMAND_POOL_USAGE_GRAPHICS;
    uint32_t QueueFamilyIndex          = UINT32_MAX;
};

class VulkanCommandPool final : private Uncopyable, private Unmovable
{
  public:
    VulkanCommandPool(const CommandPoolSpecification& commandPoolSpecification);
    VulkanCommandPool()  = delete;
    ~VulkanCommandPool() = default;

    FORCEINLINE const auto& GetCommandBuffers() const { return m_CommandBuffers; }
    FORCEINLINE auto& GetCommandBuffers() { return m_CommandBuffers; }

    FORCEINLINE const auto& Get() const { return m_CommandPool; }
    FORCEINLINE auto& Get() { return m_CommandPool; }

    void FreeCommandBuffers(std::vector<Ref<VulkanCommandBuffer>>& commandBuffers);
    void AllocateCommandBuffers(std::vector<Ref<VulkanCommandBuffer>>& outCommandBuffers, const uint32_t size,
                                VkCommandBufferLevel commandBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    void AllocateSingleCommandBuffer(VkCommandBuffer& commandBuffer,
                                     VkCommandBufferLevel commandBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    void FreeSingleCommandBuffer(VkCommandBuffer& commandBuffer);

    void Destroy();

  private:
    VkCommandPool m_CommandPool;
    CommandPoolSpecification m_CommandPoolSpecification;
    std::vector<Ref<VulkanCommandBuffer>> m_CommandBuffers;

    void CreateCommandPool();
};
}  // namespace Gauntlet
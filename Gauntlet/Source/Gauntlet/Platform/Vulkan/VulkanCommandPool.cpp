#include "GauntletPCH.h"
#include "VulkanCommandPool.h"

#include "VulkanUtility.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"

namespace Gauntlet
{

VulkanCommandPool::VulkanCommandPool(const CommandPoolSpecification& commandPoolSpecification)
    : m_CommandPoolSpecification(commandPoolSpecification)
{
    CreateCommandPool();
    AllocateCommandBuffers(m_CommandBuffers, FRAMES_IN_FLIGHT);
}

void VulkanCommandPool::CreateCommandPool()
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    VkCommandPoolCreateInfo commandPoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    commandPoolCreateInfo.flags =
        CommandPoolSpecification::ConvertCommandPoolUsageFlagsToVulkan(m_CommandPoolSpecification.CommandPoolUsage);
    commandPoolCreateInfo.queueFamilyIndex = m_CommandPoolSpecification.QueueFamilyIndex;

    VK_CHECK(vkCreateCommandPool(context.GetDevice()->GetLogicalDevice(), &commandPoolCreateInfo, nullptr, &m_CommandPool),
             "Failed to create command pool!");
}

void VulkanCommandPool::FreeCommandBuffers(std::vector<VulkanCommandBuffer>& commandBuffers)
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    std::vector<VkCommandBuffer> cmdBuffers(commandBuffers.size());
    for (uint32_t i = 0; i < cmdBuffers.size(); ++i)
        cmdBuffers[i] = commandBuffers[i].Get();

    commandBuffers.clear();
    vkFreeCommandBuffers(context.GetDevice()->GetLogicalDevice(), m_CommandPool, static_cast<uint32_t>(cmdBuffers.size()),
                         cmdBuffers.data());
}

void VulkanCommandPool::AllocateCommandBuffers(std::vector<VulkanCommandBuffer>& secondaryCommandBuffers, const uint32_t size,
                                               VkCommandBufferLevel commandBufferLevel)
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    commandBufferAllocateInfo.commandPool                 = m_CommandPool;
    commandBufferAllocateInfo.level                       = commandBufferLevel;
    commandBufferAllocateInfo.commandBufferCount          = 1;

    secondaryCommandBuffers.resize(size);
    for (auto& commandBuffer : secondaryCommandBuffers)
    {
        commandBuffer.m_Level = commandBufferLevel;
        VK_CHECK(vkAllocateCommandBuffers(context.GetDevice()->GetLogicalDevice(), &commandBufferAllocateInfo, &commandBuffer.Get()),
                 "Failed to allocate command buffer!");
    }
}

void VulkanCommandPool::Destroy()
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    vkDestroyCommandPool(context.GetDevice()->GetLogicalDevice(), m_CommandPool, nullptr);
}

}  // namespace Gauntlet
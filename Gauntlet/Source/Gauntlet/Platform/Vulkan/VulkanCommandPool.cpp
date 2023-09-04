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
    AllocatePrimaryCommandBuffers();
}

void VulkanCommandPool::CreateCommandPool()
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    VkCommandPoolCreateInfo CommandPoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    CommandPoolCreateInfo.flags =
        CommandPoolSpecification::ConvertCommandPoolUsageFlagsToVulkan(m_CommandPoolSpecification.CommandPoolUsage);
    CommandPoolCreateInfo.queueFamilyIndex = m_CommandPoolSpecification.QueueFamilyIndex;

    VK_CHECK(vkCreateCommandPool(context.GetDevice()->GetLogicalDevice(), &CommandPoolCreateInfo, nullptr, &m_CommandPool),
             "Failed to create command pool!");
}

void VulkanCommandPool::AllocatePrimaryCommandBuffers()
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    m_CommandBuffers.resize(FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    CommandBufferAllocateInfo.commandPool                 = m_CommandPool;
    CommandBufferAllocateInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CommandBufferAllocateInfo.commandBufferCount          = 1;

    for (auto& CommandBuffer : m_CommandBuffers)
    {
        VK_CHECK(vkAllocateCommandBuffers(context.GetDevice()->GetLogicalDevice(), &CommandBufferAllocateInfo, &CommandBuffer.Get()),
                 "Failed to allocate primary command buffer!");
    }
}

void VulkanCommandPool::AllocateSecondaryCommandBuffers(std::vector<VulkanCommandBuffer>& secondaryCommandBuffers)
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    commandBufferAllocateInfo.commandPool                 = m_CommandPool;
    commandBufferAllocateInfo.level                       = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    commandBufferAllocateInfo.commandBufferCount          = 1;

    for (auto& commandBuffer : secondaryCommandBuffers)
    {
        commandBuffer.m_Level = commandBufferAllocateInfo.level;
        VK_CHECK(vkAllocateCommandBuffers(context.GetDevice()->GetLogicalDevice(), &commandBufferAllocateInfo, &commandBuffer.Get()),
                 "Failed to allocate secondary command buffer!");
    }
}

void VulkanCommandPool::Destroy()
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    vkDestroyCommandPool(context.GetDevice()->GetLogicalDevice(), m_CommandPool, nullptr);
}

}  // namespace Gauntlet
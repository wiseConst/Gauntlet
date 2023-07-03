#include "EclipsePCH.h"
#include "VulkanCommandPool.h"

#include "VulkanContext.h"
#include "VulkanDevice.h"

namespace Eclipse
{

VulkanCommandPool::VulkanCommandPool(const CommandPoolSpecification& InCommandPoolSpecification)
    : m_CommandPoolSpecification(InCommandPoolSpecification)
{
    CreateCommandPool();
    AllocateCommandBuffers();
}

void VulkanCommandPool::CreateCommandPool()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    VkCommandPoolCreateInfo CommandPoolCreateInfo = {};
    CommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

    CommandPoolCreateInfo.flags =
        CommandPoolSpecification::ConvertCommandPoolUsageFlagsToVulkan(m_CommandPoolSpecification.CommandPoolUsage);
    CommandPoolCreateInfo.queueFamilyIndex = m_CommandPoolSpecification.QueueFamilyIndex;

    VK_CHECK(vkCreateCommandPool(Context.GetDevice()->GetLogicalDevice(), &CommandPoolCreateInfo, nullptr, &m_CommandPool),
             "Failed to create command pool!");
}

void VulkanCommandPool::AllocateCommandBuffers()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    m_CommandBuffers.resize(FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {};
    CommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CommandBufferAllocateInfo.commandPool = m_CommandPool;
    CommandBufferAllocateInfo.commandBufferCount = 1;
    CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    for (auto& CommandBuffer : m_CommandBuffers)
    {
        VK_CHECK(vkAllocateCommandBuffers(Context.GetDevice()->GetLogicalDevice(), &CommandBufferAllocateInfo, &CommandBuffer.Get()),
                 "Failed to allocate command buffers!");
    }
}

void VulkanCommandPool::Destroy()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    vkDestroyCommandPool(Context.GetDevice()->GetLogicalDevice(), m_CommandPool, nullptr);
}

}  // namespace Eclipse
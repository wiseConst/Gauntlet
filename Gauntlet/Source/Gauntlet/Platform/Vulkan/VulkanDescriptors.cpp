#include "GauntletPCH.h"
#include "VulkanDescriptors.h"

#include "VulkanDevice.h"
#include "VulkanUtility.h"

namespace Gauntlet
{
// VulkanDescriptorAllocator

VulkanDescriptorAllocator::VulkanDescriptorAllocator(Scoped<VulkanDevice>& InDevice) : m_Device(InDevice) {}

bool VulkanDescriptorAllocator::Allocate(VkDescriptorSet* InDescriptorSet, VkDescriptorSetLayout InDescriptorSetLayout)
{
    // Initialize the currentPool handle if it's null
    if (m_CurrentPool == VK_NULL_HANDLE)
    {
        if (m_Pools.size() > 0)
        {
            m_CurrentPool = m_Pools.back();
        }
        else
        {
            m_CurrentPool = CreatePool(m_CurrentPoolSizeMultiplier, 0);
            m_Pools.push_back(m_CurrentPool);
            m_CurrentPoolSizeMultiplier *= 1.3;  // TODO: Use dynamically calculated multiplier depending on m_Pools.size()
        }
    }

    const auto DescriptorSetAllocateInfo = Utility::GetDescriptorSetAllocateInfo(m_CurrentPool, 1, &InDescriptorSetLayout);

    // Try to allocate the descriptor set
    auto AllocationResult = vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &DescriptorSetAllocateInfo, InDescriptorSet);
    if (AllocationResult == VK_SUCCESS) return true;

    // Allocation failed try to get new pool and allocate using new pool
    if (AllocationResult == VK_ERROR_FRAGMENTED_POOL || AllocationResult == VK_ERROR_OUT_OF_POOL_MEMORY)
    {
        m_CurrentPool = CreatePool(m_CurrentPoolSizeMultiplier, 0);
        m_Pools.push_back(m_CurrentPool);
        m_CurrentPoolSizeMultiplier *= 1.3;

        // If it still fails then we have big issues
        AllocationResult = vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &DescriptorSetAllocateInfo, InDescriptorSet);
        if (AllocationResult == VK_SUCCESS) return true;
    }

    GNT_ASSERT(false, "Failed to allocate descriptor pool!");
    return false;
}

VkDescriptorPool VulkanDescriptorAllocator::CreatePool(const uint32_t InCount, VkDescriptorPoolCreateFlags InDescriptorPoolCreateFlags)
{
    std::vector<VkDescriptorPoolSize> PoolSizes(m_DefaultPoolSizes.size());

    for (uint32_t i = 0; i < PoolSizes.size(); ++i)
    {
        PoolSizes[i].type            = m_DefaultPoolSizes[i].first;
        PoolSizes[i].descriptorCount = m_DefaultPoolSizes[i].second * InCount;
    }

    const auto DescriptorPoolCreateInfo = Utility::GetDescriptorPoolCreateInfo(static_cast<uint32_t>(PoolSizes.size()), InCount,
                                                                               PoolSizes.data(), InDescriptorPoolCreateFlags);

    VkDescriptorPool NewDescriptorPool = VK_NULL_HANDLE;
    VK_CHECK(vkCreateDescriptorPool(m_Device->GetLogicalDevice(), &DescriptorPoolCreateInfo, nullptr, &NewDescriptorPool),
             "Failed to create descriptor pool!");

    return NewDescriptorPool;
}

void VulkanDescriptorAllocator::ResetPools()
{
    m_Device->WaitDeviceOnFinish();

    for (auto& Pool : m_Pools)
        vkResetDescriptorPool(m_Device->GetLogicalDevice(), Pool, 0);

    m_CurrentPool               = VK_NULL_HANDLE;
    m_CurrentPoolSizeMultiplier = m_BasePoolSizeMultiplier;
}

void VulkanDescriptorAllocator::Destroy()
{
    for (auto& Pool : m_Pools)
        vkDestroyDescriptorPool(m_Device->GetLogicalDevice(), Pool, nullptr);

    m_CurrentPool = VK_NULL_HANDLE;
}

// VulkanDescriptorBuilder

}  // namespace Gauntlet
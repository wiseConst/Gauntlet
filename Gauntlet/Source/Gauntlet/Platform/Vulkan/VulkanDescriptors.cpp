#include "GauntletPCH.h"
#include "VulkanDescriptors.h"

#include "VulkanDevice.h"
#include "VulkanUtility.h"

#include "Gauntlet/Renderer/Renderer.h"

namespace Gauntlet
{

VulkanDescriptorAllocator::VulkanDescriptorAllocator(Scoped<VulkanDevice>& device) : m_Device(device) {}

bool VulkanDescriptorAllocator::Allocate(DescriptorSet& outDescriptorSet, VkDescriptorSetLayout descriptorSetLayout)
{
    GRAPHICS_GUARD_LOCK;

    // Initialize the currentPool handle if it's null
    if (m_CurrentPool == VK_NULL_HANDLE)
    {
        if (m_Pools.size() > 0)
        {
            m_CurrentPool = m_Pools.back();
        }
        else
        {
            m_CurrentPool = CreatePool(m_CurrentPoolSizeMultiplier);
            m_Pools.push_back(m_CurrentPool);
            m_CurrentPoolSizeMultiplier =
                (uint32_t)(m_CurrentPoolSizeMultiplier * 1.3f);  // TODO: Use dynamically calculated multiplier depending on m_Pools.size()
        }
    }

    VkResult AllocationResult = VK_RESULT_MAX_ENUM;

    // Try to allocate the descriptor set with current
    {
        const auto CurrentDescriptorSetAllocateInfo = Utility::GetDescriptorSetAllocateInfo(m_CurrentPool, 1, &descriptorSetLayout);
        AllocationResult =
            vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &CurrentDescriptorSetAllocateInfo, &outDescriptorSet.Handle);
        if (AllocationResult == VK_SUCCESS)
        {
            ++m_AllocatedDescriptorSets;
            outDescriptorSet.PoolID                      = static_cast<uint32_t>(m_Pools.size()) - 1;
            Renderer::GetStats().AllocatedDescriptorSets = m_AllocatedDescriptorSets;
            return true;
        }
    }

    // Try different pools
    for (uint32_t i = 0; i < m_Pools.size(); ++i)
    {
        // Skip current cuz it's checked already.
        if (m_Pools[i] == m_CurrentPool) continue;

        const auto NewDescriptorSetAllocateInfo = Utility::GetDescriptorSetAllocateInfo(m_Pools[i], 1, &descriptorSetLayout);

        // Try to allocate the descriptor set with different pools
        AllocationResult = vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &NewDescriptorSetAllocateInfo, &outDescriptorSet.Handle);
        if (AllocationResult == VK_SUCCESS)
        {
            ++m_AllocatedDescriptorSets;
            outDescriptorSet.PoolID                      = i;
            Renderer::GetStats().AllocatedDescriptorSets = m_AllocatedDescriptorSets;
            return true;
        }
    }

    // Allocation through all pools failed, try to get new pool and allocate using this new pool
    if (AllocationResult == VK_ERROR_FRAGMENTED_POOL || AllocationResult == VK_ERROR_OUT_OF_POOL_MEMORY)
    {
        m_CurrentPool = CreatePool(m_CurrentPoolSizeMultiplier);
        m_Pools.push_back(m_CurrentPool);
        m_CurrentPoolSizeMultiplier = (uint32_t)(m_CurrentPoolSizeMultiplier * 1.3f);

        // If it still fails then we have big issues
        const auto NewDescriptorSetAllocateInfo = Utility::GetDescriptorSetAllocateInfo(m_CurrentPool, 1, &descriptorSetLayout);
        AllocationResult = vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &NewDescriptorSetAllocateInfo, &outDescriptorSet.Handle);
        if (AllocationResult == VK_SUCCESS)
        {
            ++m_AllocatedDescriptorSets;
            outDescriptorSet.PoolID                      = static_cast<uint32_t>(m_Pools.size()) - 1;
            Renderer::GetStats().AllocatedDescriptorSets = m_AllocatedDescriptorSets;
            return true;
        }
    }

    GNT_ASSERT(false, "Failed to allocate descriptor pool!");
    return false;
}

void VulkanDescriptorAllocator::ReleaseDescriptorSets(DescriptorSet* descriptorSets, const uint32_t descriptorSetCount)
{
    GRAPHICS_GUARD_LOCK;

    // Since descriptor sets can be allocated through different pool I have to iterate them like dumb
    for (uint32_t i = 0; i < descriptorSetCount; ++i)
    {
        GNT_ASSERT(descriptorSets[i].PoolID >= 0 && descriptorSets[i].PoolID < UINT32_MAX, "Invalid Pool ID!");
        if (!descriptorSets[i].Handle)
        {
            LOG_WARN("Failed to release descriptor set!");
            continue;
        }

        VK_CHECK(vkFreeDescriptorSets(m_Device->GetLogicalDevice(), m_Pools[descriptorSets[i].PoolID], 1, &descriptorSets[i].Handle),
                 "Failed to free descriptor set!");
        descriptorSets[i].Handle = VK_NULL_HANDLE;
        --m_AllocatedDescriptorSets;
    }
    Renderer::GetStats().AllocatedDescriptorSets = m_AllocatedDescriptorSets;
}

VkDescriptorPool VulkanDescriptorAllocator::CreatePool(const uint32_t count, VkDescriptorPoolCreateFlags descriptorPoolCreateFlags)
{
    std::vector<VkDescriptorPoolSize> PoolSizes(m_DefaultPoolSizes.size());

    for (uint32_t i = 0; i < PoolSizes.size(); ++i)
    {
        PoolSizes[i].type            = m_DefaultPoolSizes[i].first;
        PoolSizes[i].descriptorCount = static_cast<uint32_t>(m_DefaultPoolSizes[i].second * count);
    }

    const auto DescriptorPoolCreateInfo =
        Utility::GetDescriptorPoolCreateInfo(static_cast<uint32_t>(PoolSizes.size()), count, PoolSizes.data(),
                                             VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | descriptorPoolCreateFlags);

    VkDescriptorPool NewDescriptorPool = VK_NULL_HANDLE;
    VK_CHECK(vkCreateDescriptorPool(m_Device->GetLogicalDevice(), &DescriptorPoolCreateInfo, nullptr, &NewDescriptorPool),
             "Failed to create descriptor pool!");

    return NewDescriptorPool;
}

void VulkanDescriptorAllocator::ResetPools()
{
    GRAPHICS_GUARD_LOCK;

    m_Device->WaitDeviceOnFinish();

    m_AllocatedDescriptorSets                    = 0;
    Renderer::GetStats().AllocatedDescriptorSets = m_AllocatedDescriptorSets;

    for (auto& Pool : m_Pools)
        vkResetDescriptorPool(m_Device->GetLogicalDevice(), Pool, 0);

    m_CurrentPool               = VK_NULL_HANDLE;
    m_CurrentPoolSizeMultiplier = m_BasePoolSizeMultiplier;
}

void VulkanDescriptorAllocator::Destroy()
{
    // ResetPools(); Pool reset done via destroying pool

    m_Device->WaitDeviceOnFinish();

    for (auto& Pool : m_Pools)
        vkDestroyDescriptorPool(m_Device->GetLogicalDevice(), Pool, nullptr);

    m_CurrentPool               = VK_NULL_HANDLE;
    m_AllocatedDescriptorSets   = 0;
    m_CurrentPoolSizeMultiplier = m_BasePoolSizeMultiplier;
}

}  // namespace Gauntlet
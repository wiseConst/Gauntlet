#include "GauntletPCH.h"
#include "VulkanDescriptors.h"

#include "VulkanDevice.h"
#include "VulkanUtility.h"

#include "Gauntlet/Renderer/Renderer.h"

namespace Gauntlet
{

// TODO: Since I set VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT which specifies that I can free individual descriptor sets instead of
// full pool, I have to add void FreeDescriptorSet func here.

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
            m_CurrentPoolSizeMultiplier *= 1.3f;  // TODO: Use dynamically calculated multiplier depending on m_Pools.size()
        }
    }

    const auto CurrentDescriptorSetAllocateInfo = Utility::GetDescriptorSetAllocateInfo(m_CurrentPool, 1, &descriptorSetLayout);

    // Try to allocate the descriptor set
    auto AllocationResult =
        vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &CurrentDescriptorSetAllocateInfo, &outDescriptorSet.Handle);
    if (AllocationResult == VK_SUCCESS)
    {
        ++m_AllocatedDescriptorSets;
        outDescriptorSet.PoolID                      = m_Pools.size() - 1;
        Renderer::GetStats().AllocatedDescriptorSets = m_AllocatedDescriptorSets;
        return true;
    }

    // Allocation failed, try to get new pool and allocate using this new pool
    if (AllocationResult == VK_ERROR_FRAGMENTED_POOL || AllocationResult == VK_ERROR_OUT_OF_POOL_MEMORY)
    {
        m_CurrentPool = CreatePool(m_CurrentPoolSizeMultiplier);
        m_Pools.push_back(m_CurrentPool);
        m_CurrentPoolSizeMultiplier *= 1.3f;

        // If it still fails then we have big issues
        const auto NewDescriptorSetAllocateInfo = Utility::GetDescriptorSetAllocateInfo(m_CurrentPool, 1, &descriptorSetLayout);
        AllocationResult = vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &NewDescriptorSetAllocateInfo, &outDescriptorSet.Handle);
        if (AllocationResult == VK_SUCCESS)
        {
            ++m_AllocatedDescriptorSets;
            outDescriptorSet.PoolID                      = m_Pools.size() - 1;
            Renderer::GetStats().AllocatedDescriptorSets = m_AllocatedDescriptorSets;
            return true;
        }
    }

    GNT_ASSERT(false, "Failed to allocate descriptor pool!");
    return false;
}

void VulkanDescriptorAllocator::ReleaseDescriptorSets(DescriptorSet* descriptorSets, const uint32_t descriptorSetCount)
{
    // Since descriptor sets can be allocated through different pool I have to iterate them like dumb
    for (uint32_t i = 0; i < descriptorSetCount; ++i)
    {
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
        PoolSizes[i].descriptorCount = m_DefaultPoolSizes[i].second * count;
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
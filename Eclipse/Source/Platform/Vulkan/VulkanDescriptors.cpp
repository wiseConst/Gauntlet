#include "EclipsePCH.h"
#include "VulkanDescriptors.h"

#include "VulkanDevice.h"

namespace Eclipse
{
// ###########################################
//          VulkanDescriptorAllocator
// ###########################################
VulkanDescriptorAllocator::VulkanDescriptorAllocator(Scoped<VulkanDevice>& InDevice) : m_Device(InDevice) {}

void VulkanDescriptorAllocator::ResetPools()
{
    // Reset all active pools and add them to the free pools
    for (auto& UsedPool : m_ActivePools)
    {
        VK_CHECK(vkResetDescriptorPool(m_Device->GetLogicalDevice(), UsedPool, 0), "Failed to reset descriptor pool!");
    }

    // Clear the active pools, since we've put them all in the free pools
    m_FreePools = m_ActivePools;
    m_ActivePools.clear();

    // Reset the current pool handle back to null
    m_CurrentPool = VK_NULL_HANDLE;
}

bool VulkanDescriptorAllocator::Allocate(VkDescriptorSet* InDescriptorSet, VkDescriptorSetLayout InDescriptorSetLayout)
{
    // Initialize the currentPool handle if it's null
    if (m_CurrentPool == VK_NULL_HANDLE)
    {
        m_CurrentPool = GrabFreePool();
        m_ActivePools.push_back(m_CurrentPool);
    }

    VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo = Utility::GetDescriptorSetAllocateInfo(m_CurrentPool, 1, &InDescriptorSetLayout);

    // Try to allocate the descriptor set
    auto AllocationResult = vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &DescriptorSetAllocateInfo, InDescriptorSet);
    bool bNeedReallocate  = false;

    switch (AllocationResult)
    {
        case VK_SUCCESS: return true;
        case VK_ERROR_FRAGMENTED_POOL:
        case VK_ERROR_OUT_OF_POOL_MEMORY: bNeedReallocate = true; break;
        default: ELS_ASSERT(false, "Unknown allocation result!"); return false;
    }

    if (bNeedReallocate)
    {
        // Allocate a new pool and retry
        m_CurrentPool = GrabFreePool();
        m_ActivePools.push_back(m_CurrentPool);

        AllocationResult = vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &DescriptorSetAllocateInfo, InDescriptorSet);

        // If it still fails then we have big issues
        if (AllocationResult == VK_SUCCESS)
        {
            return true;
        }
    }

    return false;
}

VkDescriptorPool VulkanDescriptorAllocator::CreatePool(const uint32_t InCount,
                                                       VkDescriptorPoolCreateFlags InDescriptorPoolCreateFlags) const
{
    std::vector<VkDescriptorPoolSize> Sizes;
    Sizes.reserve(m_PoolSizes.Sizes.size());
    for (auto PoolSize : m_PoolSizes.Sizes)
    {
        Sizes.push_back({PoolSize.first, uint32_t(PoolSize.second * InCount)});
    }

    VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo = {};
    DescriptorPoolCreateInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    DescriptorPoolCreateInfo.maxSets                    = InCount;
    DescriptorPoolCreateInfo.poolSizeCount              = (uint32_t)Sizes.size();
    DescriptorPoolCreateInfo.pPoolSizes                 = Sizes.data();
    DescriptorPoolCreateInfo.flags                      = InDescriptorPoolCreateFlags;

    VkDescriptorPool DescriptorPool = VK_NULL_HANDLE;
    VK_CHECK(vkCreateDescriptorPool(m_Device->GetLogicalDevice(), &DescriptorPoolCreateInfo, nullptr, &DescriptorPool),
             "Failed to create descriptor pool!");

    return DescriptorPool;
}

VkDescriptorPool VulkanDescriptorAllocator::GrabFreePool() const
{
    if (m_FreePools.size() > 0)
    {
        VkDescriptorPool Pool = m_FreePools.back();
        return Pool;
    }

    return CreatePool(1000, 0);
}

void VulkanDescriptorAllocator::Destroy()
{
    for (auto& FreePool : m_FreePools)
    {
        vkDestroyDescriptorPool(m_Device->GetLogicalDevice(), FreePool, nullptr);
    }

    for (auto& ActivePool : m_ActivePools)
    {
        vkDestroyDescriptorPool(m_Device->GetLogicalDevice(), ActivePool, nullptr);
    }
}

// ###########################################
//          VulkanDescriptorLayoutCache
// ###########################################
VulkanDescriptorLayoutCache::VulkanDescriptorLayoutCache(Scoped<VulkanDevice>& InDevice) : m_Device(InDevice) {}

VkDescriptorSetLayout VulkanDescriptorLayoutCache::CreateDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo* info)
{
    DescriptorLayoutInfo LayoutInfo = {};
    LayoutInfo.Bindings.reserve(info->bindingCount);

    // Bindings should be in ascending order
    bool bIsSorted      = true;
    int32_t LastBinding = -1;
    for (uint32_t i = 0; i < info->bindingCount; i++)
    {
        LayoutInfo.Bindings.push_back(info->pBindings[i]);

        if (info->pBindings[i].binding > LastBinding)
        {
            LastBinding = info->pBindings[i].binding;
        }
        else
        {
            bIsSorted = false;
        }
    }

    if (!bIsSorted)
    {
        std::sort(LayoutInfo.Bindings.begin(), LayoutInfo.Bindings.end(),
                  [](const VkDescriptorSetLayoutBinding& a, const VkDescriptorSetLayoutBinding& b) { return a.binding < b.binding; });
    }

    // Check first if it's already in the cache and grab it if it is.
    const auto it = m_LayoutCache.find(LayoutInfo);
    if (it != m_LayoutCache.end())
    {
        return (*it).second;
    }

    // Create a new one if not found
    VkDescriptorSetLayout Layout;
    VK_CHECK(vkCreateDescriptorSetLayout(m_Device->GetLogicalDevice(), info, nullptr, &Layout), "Failed to create descriptor set layout!");

    // Add to cache
    m_LayoutCache[LayoutInfo] = Layout;
    return Layout;
}

bool VulkanDescriptorLayoutCache::DescriptorLayoutInfo::operator==(const DescriptorLayoutInfo& other) const
{
    if (other.Bindings.size() != Bindings.size()) return false;

    for (int i = 0; i < Bindings.size(); i++)
    {
        if (other.Bindings[i].binding != Bindings[i].binding) return false;

        if (other.Bindings[i].descriptorType != Bindings[i].descriptorType) return false;

        if (other.Bindings[i].descriptorCount != Bindings[i].descriptorCount) return false;

        if (other.Bindings[i].stageFlags != Bindings[i].stageFlags) return false;
    }

    return true;
}

size_t VulkanDescriptorLayoutCache::DescriptorLayoutInfo::hash() const
{
    using std::hash;
    using std::size_t;

    size_t result = hash<size_t>()(Bindings.size());
    for (const auto& Binding : Bindings)
    {
        const size_t BindingHash = Binding.binding | Binding.descriptorType << 8 | Binding.descriptorCount << 16 | Binding.stageFlags << 24;
        result ^= hash<size_t>()(BindingHash);
    }

    return result;
}

void VulkanDescriptorLayoutCache::Destroy()
{
    for (auto& Pair : m_LayoutCache)
    {
        vkDestroyDescriptorSetLayout(m_Device->GetLogicalDevice(), Pair.second, nullptr);
    }
}
// ###########################################
//          DescriptorBuilder
// ##########################################
DescriptorBuilder DescriptorBuilder::Begin(Ref<VulkanDescriptorLayoutCache>& InDescriptorLayoutCache,
                                           Ref<VulkanDescriptorAllocator>& InDescriptorAllocator)
{
    DescriptorBuilder Builder       = {};
    Builder.m_DescriptorLayoutCache = InDescriptorLayoutCache;
    Builder.m_DescriptorAllocator   = InDescriptorAllocator;

    return Builder;
}

DescriptorBuilder& DescriptorBuilder::BindBuffer(const uint32_t InBinding, VkDescriptorBufferInfo* InBufferInfo,
                                                 VkDescriptorType InDescriptorType, VkShaderStageFlags InShaderStageFlags,
                                                 const uint32_t InDescriptorCount)
{
    // Create the descriptor binding for the layout
    VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding =
        Utility::GetDescriptorSetLayoutBinding(InBinding, 1, InDescriptorType, InShaderStageFlags);

    m_Bindings.push_back(DescriptorSetLayoutBinding);

    // Create the descriptor write
    VkWriteDescriptorSet WriteDescriptorSet =
        Utility::GetWriteDescriptorSet(InDescriptorType, InBinding, {}, InDescriptorCount, InBufferInfo);
    m_Writes.push_back(WriteDescriptorSet);
    return *this;
}

DescriptorBuilder& DescriptorBuilder::BindImage(const uint32_t InBinding, VkDescriptorImageInfo* InImageInfo,
                                                VkDescriptorType InDescriptorType, VkShaderStageFlags InShaderStageFlags,
                                                const uint32_t InDescriptorCount)
{
    // Create the descriptor binding for the layout
    VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding =
        Utility::GetDescriptorSetLayoutBinding(InBinding, 1, InDescriptorType, InShaderStageFlags);

    m_Bindings.push_back(DescriptorSetLayoutBinding);

    // Create the descriptor write
    VkWriteDescriptorSet WriteDescriptorSet =
        Utility::GetWriteDescriptorSet(InDescriptorType, InBinding, {}, InDescriptorCount, InImageInfo);
    m_Writes.push_back(WriteDescriptorSet);
    return *this;
}

bool DescriptorBuilder::Build(VkDescriptorSet& InDescriptorSet, VkDescriptorSetLayout& InDescriptorSetLayout)
{
    // Build layout descriptor set layout first
    VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo = {};
    DescriptorSetLayoutCreateInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    DescriptorSetLayoutCreateInfo.pBindings                       = m_Bindings.data();
    DescriptorSetLayoutCreateInfo.bindingCount                    = static_cast<uint32_t>(m_Bindings.size());

    InDescriptorSetLayout = m_DescriptorLayoutCache->CreateDescriptorSetLayout(&DescriptorSetLayoutCreateInfo);

    // Allocate descriptor
    if (!m_DescriptorAllocator->Allocate(&InDescriptorSet, InDescriptorSetLayout)) return false;

    // Write descriptor
    for (auto& Write : m_Writes)
    {
        Write.dstSet = InDescriptorSet;
    }

    vkUpdateDescriptorSets(m_DescriptorAllocator->GetVulkanDevice()->GetLogicalDevice(), m_Writes.size(), m_Writes.data(), 0, nullptr);
    return true;
}

bool DescriptorBuilder::Build(VkDescriptorSet& InDescriptorSet)
{
    VkDescriptorSetLayout DescriptorSetLayout = VK_NULL_HANDLE;
    return Build(InDescriptorSet, DescriptorSetLayout);
}

}  // namespace Eclipse
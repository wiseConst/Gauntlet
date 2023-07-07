#include "EclipsePCH.h"
#include "VulkanDescriptors.h"

#include "VulkanDevice.h"

namespace Eclipse
{
// DescriptorAllocator
DescriptorAllocator::DescriptorAllocator(Scoped<VulkanDevice>& InDevice) : m_Device(InDevice)
{
    Create();
}

void DescriptorAllocator::Create() {}

void DescriptorAllocator::ResetPools()
{
    for (auto& UsedPool : m_UsedPools)
    {
        VK_CHECK(vkResetDescriptorPool(m_Device->GetLogicalDevice(), UsedPool, 0), "Failed to reset descriptor pool!");
        m_FreePools.push_back(UsedPool);
    }

    m_UsedPools.clear();
    m_CurrentPool = VK_NULL_HANDLE;
}

bool DescriptorAllocator::Allocate(VkDescriptorSet* InDescriptorSet, VkDescriptorSetLayout InDescriptorSetLayout)
{
    if (m_CurrentPool == VK_NULL_HANDLE)
    {

        m_CurrentPool = GrabFreePool();
        m_UsedPools.push_back(m_CurrentPool);
    }

    VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo = {};
    DescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

    DescriptorSetAllocateInfo.pSetLayouts = &InDescriptorSetLayout;
    DescriptorSetAllocateInfo.descriptorPool = m_CurrentPool;
    DescriptorSetAllocateInfo.descriptorSetCount = 1;

    auto AllocationResult = vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &DescriptorSetAllocateInfo, InDescriptorSet);
    bool needReallocate = false;

    switch (AllocationResult)
    {
        case VK_SUCCESS: return true;
        case VK_ERROR_FRAGMENTED_POOL:
        case VK_ERROR_OUT_OF_POOL_MEMORY: needReallocate = true; break;
        default: ELS_ASSERT(false, "Unknown allocation result!"); return false;
    }

    if (needReallocate)
    {
        m_CurrentPool = GrabFreePool();
        m_UsedPools.push_back(m_CurrentPool);

        AllocationResult = vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &DescriptorSetAllocateInfo, InDescriptorSet);

        if (AllocationResult == VK_SUCCESS)
        {
            return true;
        }
    }

    return false;
}

VkDescriptorPool DescriptorAllocator::CreatePool(const uint32_t InCount, VkDescriptorPoolCreateFlags InDescriptorPoolCreateFlags) const
{
    std::vector<VkDescriptorPoolSize> Sizes;
    Sizes.reserve(m_PoolSizes.Sizes.size());
    for (auto PoolSize : m_PoolSizes.Sizes)
    {
        Sizes.push_back({PoolSize.first, uint32_t(PoolSize.second * InCount)});
    }

    VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo = {};
    DescriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    DescriptorPoolCreateInfo.flags = InDescriptorPoolCreateFlags;
    DescriptorPoolCreateInfo.maxSets = InCount;
    DescriptorPoolCreateInfo.poolSizeCount = (uint32_t)Sizes.size();
    DescriptorPoolCreateInfo.pPoolSizes = Sizes.data();

    VkDescriptorPool DescriptorPool = VK_NULL_HANDLE;
    VK_CHECK(vkCreateDescriptorPool(m_Device->GetLogicalDevice(), &DescriptorPoolCreateInfo, nullptr, &DescriptorPool),
             "Failed to create descriptor pool!");

    return DescriptorPool;
}

VkDescriptorPool DescriptorAllocator::GrabFreePool() const
{
    if (m_FreePools.size() > 0)
    {
        VkDescriptorPool Pool = m_FreePools.back();
        return Pool;
    }

    return CreatePool(1000, 0);
}

void DescriptorAllocator::Destroy()
{
    for (auto& FreePool : m_FreePools)
    {
        vkDestroyDescriptorPool(m_Device->GetLogicalDevice(), FreePool, nullptr);
    }

    for (auto& UsedPool : m_UsedPools)
    {
        vkDestroyDescriptorPool(m_Device->GetLogicalDevice(), UsedPool, nullptr);
    }
}

// DescriptorLayoutCache

DescriptorLayoutCache::DescriptorLayoutCache(Scoped<VulkanDevice>& InDevice) : m_Device(InDevice)
{
    Create();
}

void DescriptorLayoutCache::Create() {}

VkDescriptorSetLayout DescriptorLayoutCache::CreateDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo* info)
{
    DescriptorLayoutInfo LayoutInfo = {};
    LayoutInfo.Bindings.reserve(info->bindingCount);
    bool bIsSorted = true;
    int LastBinding = -1;

    for (int i = 0; i < info->bindingCount; i++)
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

    // try to grab from cache
    auto it = m_LayoutCache.find(LayoutInfo);
    if (it != m_LayoutCache.end())
    {
        return (*it).second;
    }
    // create a new one (not found)
    VkDescriptorSetLayout Layout;
    VK_CHECK(vkCreateDescriptorSetLayout(m_Device->GetLogicalDevice(), info, nullptr, &Layout), "Failed to create descriptor set layout!");

    m_LayoutCache[LayoutInfo] = Layout;
    return Layout;
}

bool DescriptorLayoutCache::DescriptorLayoutInfo::operator==(const DescriptorLayoutInfo& other) const
{
    if (other.Bindings.size() != Bindings.size())
    {
        return false;
    }

    for (int i = 0; i < Bindings.size(); i++)
    {
        if (other.Bindings[i].binding != Bindings[i].binding)
        {
            return false;
        }

        if (other.Bindings[i].descriptorType != Bindings[i].descriptorType)
        {
            return false;
        }

        if (other.Bindings[i].descriptorCount != Bindings[i].descriptorCount)
        {
            return false;
        }

        if (other.Bindings[i].stageFlags != Bindings[i].stageFlags)
        {
            return false;
        }
    }
    return true;
}

size_t DescriptorLayoutCache::DescriptorLayoutInfo::hash() const
{
    using std::hash;
    using std::size_t;

    size_t result = hash<size_t>()(Bindings.size());
    for (const VkDescriptorSetLayoutBinding& Binding : Bindings)
    {
        const size_t binding_hash =
            Binding.binding | Binding.descriptorType << 8 | Binding.descriptorCount << 16 | Binding.stageFlags << 24;
        result ^= hash<size_t>()(binding_hash);
    }

    return result;
}

void DescriptorLayoutCache::Destroy()
{
    for (auto& Pair : m_LayoutCache)
    {
        vkDestroyDescriptorSetLayout(m_Device->GetLogicalDevice(), Pair.second, nullptr);
    }
}

// DescriptorBuilder

DescriptorBuilder DescriptorBuilder::Begin(Ref<DescriptorLayoutCache>& InDescriptorLayoutCache,
                                           Ref<DescriptorAllocator>& InDescriptorAllocator)
{
    DescriptorBuilder Builder;

    Builder.m_DescriptorLayoutCache = InDescriptorLayoutCache;
    Builder.m_DescriptorAllocator = InDescriptorAllocator;
    return Builder;
}

DescriptorBuilder& DescriptorBuilder::BindBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type,
                                                 VkShaderStageFlags stageFlags)
{
    // VkDescriptorSetLayoutBinding newBinding{};

    // newBinding.descriptorCount = 1;
    // newBinding.descriptorType = type;
    // newBinding.pImmutableSamplers = nullptr;
    // newBinding.stageFlags = stageFlags;
    // newBinding.binding = binding;

    // bindings.push_back(newBinding);

    //// create the descriptor write
    // VkWriteDescriptorSet newWrite{};
    // newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    // newWrite.pNext = nullptr;

    // newWrite.descriptorCount = 1;
    // newWrite.descriptorType = type;
    // newWrite.pBufferInfo = bufferInfo;
    // newWrite.dstBinding = binding;

    // writes.push_back(newWrite);
    return *this;
}

DescriptorBuilder& DescriptorBuilder::BindImage(uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type,
                                                VkShaderStageFlags stageFlags)
{
    return *this;
}

bool DescriptorBuilder::Build(VkDescriptorSet& set, VkDescriptorSetLayout& layout)
{
    // VkDescriptorSetLayoutCreateInfo layoutInfo{};
    // layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    // layoutInfo.pNext = nullptr;

    // layoutInfo.pBindings = bindings.data();
    // layoutInfo.bindingCount = bindings.size();

    // layout = cache->create_descriptor_layout(&layoutInfo);

    //// allocate descriptor
    // bool success = alloc->allocate(&set, layout);
    // if (!success)
    //{
    //     return false;
    // };

    //// write descriptor
    // for (VkWriteDescriptorSet& w : writes)
    //{
    //     w.dstSet = set;
    // }

    // vkUpdateDescriptorSets(alloc->device, writes.size(), writes.data(), 0, nullptr);

    return true;
}

bool DescriptorBuilder::Build(VkDescriptorSet& set)
{
    return false;
}

}  // namespace Eclipse
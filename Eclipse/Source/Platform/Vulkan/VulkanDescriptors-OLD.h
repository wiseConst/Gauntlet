#pragma once

#include <Eclipse/Core/Core.h>
#include <volk/volk.h>

#include <vector>

// Thanks to VKGUIDE.DEV

namespace Eclipse
{
class VulkanDevice;

// VulkanDescriptorAllocator

class VulkanDescriptorAllocator final : private Uncopyable, private Unmovable
{
  public:
    VulkanDescriptorAllocator(Scoped<VulkanDevice>& InDevice);
    ~VulkanDescriptorAllocator() = default;

    void ResetPools();
    void Destroy();

    bool Allocate(VkDescriptorSet* InDescriptorSet, VkDescriptorSetLayout InDescriptorSetLayout);

    FORCEINLINE const auto& GetVulkanDevice() const { return m_Device; }
    FORCEINLINE auto& GetVulkanDevice() { return m_Device; }

  private:
    Scoped<VulkanDevice>& m_Device;
    VkDescriptorPool m_CurrentPool = VK_NULL_HANDLE;

    std::vector<VkDescriptorPool> m_ActivePools;
    std::vector<VkDescriptorPool> m_FreePools;

    struct PoolSizes
    {
        std::vector<std::pair<VkDescriptorType, float>> Sizes =  //
            {{VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f},
             {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f},
             {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f},
             {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f},
             {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f},
             {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f},
             {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f},
             {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f},
             {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f},
             {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f},
             {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f}};
    } m_PoolSizes;

    VkDescriptorPool CreatePool(const uint32_t InCount, VkDescriptorPoolCreateFlags InDescriptorPoolCreateFlags) const;
    VkDescriptorPool GrabFreePool() const;
};

// DescriptorLayoutCache

class VulkanDescriptorLayoutCache final : private Uncopyable, private Unmovable
{
  public:
    VulkanDescriptorLayoutCache(Scoped<VulkanDevice>& InDevice);
    ~VulkanDescriptorLayoutCache() = default;

    void Destroy();
    VkDescriptorSetLayout CreateDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo* info);

    struct DescriptorLayoutInfo
    {
        std::vector<VkDescriptorSetLayoutBinding> Bindings;

        bool operator==(const DescriptorLayoutInfo& other) const;

        size_t hash() const;
    };

  private:
    Scoped<VulkanDevice>& m_Device;

    struct DescriptorLayoutHash
    {
        std::size_t operator()(const DescriptorLayoutInfo& InDescriptorLayoutInfo) const { return InDescriptorLayoutInfo.hash(); }
    };

    std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> m_LayoutCache;
};

// DescriptorBuilder

class DescriptorBuilder final /*: private Uncopyable, private Unmovable*/
{
  public:
    DescriptorBuilder() = default;
    DescriptorBuilder(Ref<VulkanDescriptorLayoutCache>& InDescriptorLayoutCache, Ref<VulkanDescriptorAllocator>& InDescriptorAllocator)
        : m_DescriptorLayoutCache(InDescriptorLayoutCache), m_DescriptorAllocator(InDescriptorAllocator)
    {
    }
    ~DescriptorBuilder() = default;

    static DescriptorBuilder Begin(Ref<VulkanDescriptorLayoutCache>& InDescriptorLayoutCache,
                                   Ref<VulkanDescriptorAllocator>& InDescriptorAllocator);

    DescriptorBuilder& BindBuffer(const uint32_t InBinding, VkDescriptorBufferInfo* InBufferInfo, VkDescriptorType InDescriptorType,
                                  VkShaderStageFlags InShaderStageFlags, const uint32_t InDescriptorCount = 1);
    DescriptorBuilder& BindImage(const uint32_t InBinding, VkDescriptorImageInfo* InImageInfo, VkDescriptorType InDescriptorType,
                                 VkShaderStageFlags InShaderStageFlags, const uint32_t InDescriptorCount = 1);

    bool Build(VkDescriptorSet& InDescriptorSet, VkDescriptorSetLayout& InDescriptorSetLayout);
    bool Build(VkDescriptorSet& InDescriptorSet);

  private:
    std::vector<VkWriteDescriptorSet> m_Writes;
    std::vector<VkDescriptorSetLayoutBinding> m_Bindings;

    Ref<VulkanDescriptorLayoutCache> m_DescriptorLayoutCache;
    Ref<VulkanDescriptorAllocator> m_DescriptorAllocator;
};

}  // namespace Eclipse
#pragma once

#include <Eclipse/Core/Core.h>
#include <volk/volk.h>

#include <vector>

namespace Eclipse
{
class VulkanDevice;

// DescriptorAllocator

class DescriptorAllocator final : private Uncopyable, private Unmovable
{
  public:
    DescriptorAllocator(Scoped<VulkanDevice>& InDevice);
    ~DescriptorAllocator() = default;

    void Create();
    void Destroy();

  private:
    Scoped<VulkanDevice>& m_Device;
    VkDescriptorPool m_CurrentPool = VK_NULL_HANDLE;

    std::vector<VkDescriptorPool> m_UsedPools;
    std::vector<VkDescriptorPool> m_FreePools;

    struct PoolSizes
    {
        std::vector<std::pair<VkDescriptorType, float>> Sizes = {{VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f},
                                                                 {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f},
                                                                 {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f},
                                                                 {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f},
                                                                 {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f},
                                                                 {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f},
                                                                 {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f},
                                                                 {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f},
                                                                 {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f},
                                                                 {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f},
                                                                 {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f}};
    } m_PoolSizes;

    void ResetPools();
    bool Allocate(VkDescriptorSet* InDescriptorSet, VkDescriptorSetLayout InDescriptorSetLayout);
    VkDescriptorPool CreatePool(const uint32_t InCount, VkDescriptorPoolCreateFlags InDescriptorPoolCreateFlags) const;
    VkDescriptorPool GrabFreePool() const;
};

// DescriptorLayoutCache

class DescriptorLayoutCache final : private Uncopyable, private Unmovable
{
  public:
    DescriptorLayoutCache(Scoped<VulkanDevice>& InDevice);
    ~DescriptorLayoutCache() = default;

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

    void Create();
    void Destroy();
};

// DescriptorBuilder

class DescriptorBuilder final
{
  public:
    static DescriptorBuilder Begin(Ref<DescriptorLayoutCache>& InDescriptorLayoutCache, Ref<DescriptorAllocator>& InDescriptorAllocator);

    DescriptorBuilder& BindBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type,
                                  VkShaderStageFlags stageFlags);
    DescriptorBuilder& BindImage(uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);

    bool Build(VkDescriptorSet& set, VkDescriptorSetLayout& layout);
    bool Build(VkDescriptorSet& set);

  private:
    DescriptorBuilder() = default;
    ~DescriptorBuilder() = default;

    std::vector<VkWriteDescriptorSet> m_Writes;
    std::vector<VkDescriptorSetLayoutBinding> m_Bindings;

    Ref<DescriptorLayoutCache> m_DescriptorLayoutCache;
    Ref<DescriptorAllocator> m_DescriptorAllocator;
};

}  // namespace Eclipse
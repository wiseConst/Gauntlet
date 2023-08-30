#pragma once

#include "Gauntlet/Core/Inherited.h"
#include <volk/volk.h>

#include <vector>

namespace Gauntlet
{

class VulkanDevice;

// Simple wrapper around the whole descriptor set situation, to make them flexible around my system.
struct DescriptorSet
{
    VkDescriptorSet Handle{VK_NULL_HANDLE};  // Actual handle
    uint32_t PoolID{UINT32_MAX};                    // From which pool it is.
};

class VulkanDescriptorAllocator final : private Unmovable, private Uncopyable
{
  public:
    VulkanDescriptorAllocator(Scoped<VulkanDevice>& device);
    ~VulkanDescriptorAllocator() = default;

    NODISCARD bool Allocate(DescriptorSet& outDescriptorSet, VkDescriptorSetLayout descriptorSetLayout);
    void ReleaseDescriptorSets(DescriptorSet* descriptorSets, const uint32_t descriptorSetCount);

    void Destroy();
    void ResetPools();

    FORCEINLINE const uint32_t GetAllocatedDescriptorSetsCount() { return m_AllocatedDescriptorSets; }

  private:
    Scoped<VulkanDevice>& m_Device;

    std::vector<VkDescriptorPool> m_Pools;
    VkDescriptorPool m_CurrentPool       = VK_NULL_HANDLE;
    uint32_t m_CurrentPoolSizeMultiplier = 500;
    uint32_t m_BasePoolSizeMultiplier    = 500;
    uint32_t m_AllocatedDescriptorSets   = 0;

    std::vector<std::pair<VkDescriptorType, float>> m_DefaultPoolSizes =  //
        {{VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f},                              //
         {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f},                //
         {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f},                         //
         {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f},                         //
         {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f},                  //
         {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f},                  //
         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f},                        //
         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f},                //
         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f},                        //
         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f},                //
         {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f}};                    //

    NODISCARD VkDescriptorPool CreatePool(const uint32_t count, VkDescriptorPoolCreateFlags descriptorPoolCreateFlags = 0);
};
}  // namespace Gauntlet
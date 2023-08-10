#pragma once

#include "Gauntlet/Core/Inherited.h"
#include <volk/volk.h>

#include <vector>

namespace Gauntlet
{

class VulkanDevice;

class VulkanDescriptorAllocator final : private Unmovable, private Uncopyable
{
  public:
    VulkanDescriptorAllocator(Scoped<VulkanDevice>& InDevice);
    ~VulkanDescriptorAllocator() = default;

    bool Allocate(VkDescriptorSet* InDescriptorSet, VkDescriptorSetLayout InDescriptorSetLayout);

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

    VkDescriptorPool CreatePool(const uint32_t InCount, VkDescriptorPoolCreateFlags InDescriptorPoolCreateFlags);
};
}  // namespace Gauntlet
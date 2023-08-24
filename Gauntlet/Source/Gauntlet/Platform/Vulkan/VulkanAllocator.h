#pragma once

#include "Gauntlet/Core/Core.h"

#define VK_NO_PROTOTYPES
#include <vma/vk_mem_alloc.h>

namespace Gauntlet
{

class VulkanDevice;

class VulkanAllocator final : private Uncopyable, private Unmovable
{
  public:
    VulkanAllocator(const VkInstance& instance, const Scoped<VulkanDevice>& device);
    ~VulkanAllocator() = default;

    VmaAllocation CreateImage(const VkImageCreateInfo& imageCreateInfo, VkImage* outImage) const;
    void DestroyImage(VkImage& image, VmaAllocation& allocation) const;

    VmaAllocation CreateBuffer(const VkBufferCreateInfo& bufferCreateInfo, VkBuffer* outBuffer,
                               VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO) const;
    void DestroyBuffer(VkBuffer& buffer, VmaAllocation& allocation) const;

    void QueryAllocationInfo(VmaAllocationInfo& allocationInfo, const VmaAllocation& allocation) const;

    void* Map(VmaAllocation& allocation) const;
    void Unmap(VmaAllocation& allocation) const;

    void Destroy();

  private:
    VmaAllocator m_Allocator = VK_NULL_HANDLE;
};

}  // namespace Gauntlet
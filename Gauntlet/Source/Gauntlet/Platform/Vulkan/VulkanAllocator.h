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
    VulkanAllocator(const VkInstance& InInstance, const Scoped<VulkanDevice>& InDevice);
    ~VulkanAllocator() = default;

    VmaAllocation CreateImage(const VkImageCreateInfo& InImageCreateInfo, VkImage* InImage) const;
    void DestroyImage(VkImage& InImage, VmaAllocation& InAllocation) const;

    VmaAllocation CreateBuffer(const VkBufferCreateInfo& InBufferCreateInfo, VkBuffer* InBuffer,
                               VmaMemoryUsage InMemoryUsage = VMA_MEMORY_USAGE_AUTO) const;
    void DestroyBuffer(VkBuffer& InBuffer, VmaAllocation& InAllocation) const;

    void QueryAllocationInfo(VmaAllocationInfo& InOutAllocationInfo, const VmaAllocation& InAllocation) const;

    void* Map(VmaAllocation& InAllocation) const;
    void Unmap(VmaAllocation& InAllocation) const;

    void Destroy();

  private:
    VmaAllocator m_Allocator = VK_NULL_HANDLE;
};

}  // namespace Gauntlet
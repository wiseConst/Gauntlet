#include "EclipsePCH.h"
#include "VulkanAllocator.h"

#include "VulkanDevice.h"

namespace Eclipse
{
uint32_t VulkanAllocator::m_AllocatedImages = 0;
uint32_t VulkanAllocator::m_AllocatedBuffers = 0;

VulkanAllocator::VulkanAllocator(const VkInstance& InInstance, const Scoped<VulkanDevice>& InDevice)
{
    VmaVulkanFunctions vmaVulkanFunctions = {};
    vmaVulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
    vmaVulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;

    VmaAllocatorCreateInfo AllocatorCreateInfo = {};
    AllocatorCreateInfo.instance = InInstance;
    AllocatorCreateInfo.device = InDevice->GetLogicalDevice();
    AllocatorCreateInfo.physicalDevice = InDevice->GetPhysicalDevice();
    AllocatorCreateInfo.vulkanApiVersion = ELS_VK_API_VERSION;
    AllocatorCreateInfo.pVulkanFunctions = &vmaVulkanFunctions;

    const auto result = vmaCreateAllocator(&AllocatorCreateInfo, &m_Allocator);
    ELS_ASSERT(result == VK_SUCCESS, "Failed to create AMD Vulkan Allocator!");

    LOG_WARN("AMD Vulkan Allocator created!");
}

VmaAllocation VulkanAllocator::CreateImage(const VkImageCreateInfo& InImageCreateInfo, VkImage* InImage) const
{
    VmaAllocationCreateInfo AllocationCreateInfo = {};
    AllocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    AllocationCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    AllocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VmaAllocation Allocation = {};
    const auto result = vmaCreateImage(m_Allocator, &InImageCreateInfo, &AllocationCreateInfo, InImage, &Allocation, nullptr);
    ELS_ASSERT(result == VK_SUCCESS, "Failed to create vulkan image via VMA!");

    ++m_AllocatedImages;

    return Allocation;
}

void VulkanAllocator::DestroyImage(VkImage& InImage, VmaAllocation& InAllocation) const
{
    vmaDestroyImage(m_Allocator, InImage, InAllocation);
    --m_AllocatedImages;
}

VmaAllocation VulkanAllocator::CreateBuffer(const VkBufferCreateInfo& InBufferCreateInfo, VkBuffer* InBuffer) const
{
    VmaAllocationCreateInfo AllocationCreateInfo = {};
    AllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

    // if (InBufferCreateInfo.usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) // Staging buffer case
    //{
    AllocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    AllocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
    //}

    VmaAllocation Allocation = VK_NULL_HANDLE;
    const auto result = vmaCreateBuffer(m_Allocator, &InBufferCreateInfo, &AllocationCreateInfo, InBuffer, &Allocation, nullptr);
    ELS_ASSERT(result == VK_SUCCESS, "Failed to create buffer via VMA!");

    ++m_AllocatedBuffers;

    return Allocation;
}

void VulkanAllocator::DestroyBuffer(VkBuffer& InBuffer, VmaAllocation& InAllocation) const
{
    vmaDestroyBuffer(m_Allocator, InBuffer, InAllocation);
    --m_AllocatedBuffers;
}

void* VulkanAllocator::Map(VmaAllocation& InAllocation) const
{
    void* Mapped = nullptr;

    const auto result = vmaMapMemory(m_Allocator, InAllocation, &Mapped);
    ELS_ASSERT(result == VK_SUCCESS && Mapped, "Failed to map memory!");

    return Mapped;
}

void VulkanAllocator::Unmap(VmaAllocation& InAllocation) const
{
    vmaUnmapMemory(m_Allocator, InAllocation);
}

void VulkanAllocator::Destroy()
{
    LOG_WARN("Before VMA destroying! Remaining data: buffers (%u), images (%u).", m_AllocatedBuffers, m_AllocatedImages);
    vmaDestroyAllocator(m_Allocator);
    LOG_WARN("VMA destroyed! Remaining data: buffers (%u), images (%u).", m_AllocatedBuffers, m_AllocatedImages);
}

}  // namespace Eclipse
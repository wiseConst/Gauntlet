#include "EclipsePCH.h"
#include "VulkanAllocator.h"

#include "VulkanDevice.h"
#include "VulkanTexture.h"

#include "VulkanRenderer2D.h"

namespace Eclipse
{

VulkanAllocator::VulkanAllocator(const VkInstance& InInstance, const Scoped<VulkanDevice>& InDevice)
{
    VmaVulkanFunctions vmaVulkanFunctions    = {};
    vmaVulkanFunctions.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;
    vmaVulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;

    VmaAllocatorCreateInfo AllocatorCreateInfo = {};
    AllocatorCreateInfo.instance               = InInstance;
    AllocatorCreateInfo.device                 = InDevice->GetLogicalDevice();
    AllocatorCreateInfo.physicalDevice         = InDevice->GetPhysicalDevice();
    AllocatorCreateInfo.vulkanApiVersion       = ELS_VK_API_VERSION;
    AllocatorCreateInfo.pVulkanFunctions       = &vmaVulkanFunctions;

    const auto result = vmaCreateAllocator(&AllocatorCreateInfo, &m_Allocator);
    ELS_ASSERT(result == VK_SUCCESS, "Failed to create AMD Vulkan Allocator!");

#if ELS_DEBUG
    LOG_INFO("AMD Vulkan Allocator created!");
#endif
}

VmaAllocation VulkanAllocator::CreateImage(const VkImageCreateInfo& InImageCreateInfo, VkImage* InImage) const
{
    VmaAllocationCreateInfo AllocationCreateInfo = {};
    AllocationCreateInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
    AllocationCreateInfo.flags                   = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    AllocationCreateInfo.requiredFlags           = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VmaAllocation Allocation = {};
    const auto result        = vmaCreateImage(m_Allocator, &InImageCreateInfo, &AllocationCreateInfo, InImage, &Allocation, nullptr);
    ELS_ASSERT(result == VK_SUCCESS, "Failed to create vulkan image via VMA!");

    VmaAllocationInfo AllocationInfo = {};
    vmaGetAllocationInfo(m_Allocator, Allocation, &AllocationInfo);
    VulkanRenderer2D::GetStats().GPUMemoryAllocated += AllocationInfo.size;

    ++VulkanRenderer2D::GetStats().Allocations;
    ++VulkanRenderer2D::GetStats().AllocatedImages;
    return Allocation;
}

void VulkanAllocator::DestroyImage(VkImage& InImage, VmaAllocation& InAllocation) const
{
    VmaAllocationInfo AllocationInfo = {};
    vmaGetAllocationInfo(m_Allocator, InAllocation, &AllocationInfo);
    VulkanRenderer2D::GetStats().GPUMemoryAllocated -= AllocationInfo.size;

    vmaDestroyImage(m_Allocator, InImage, InAllocation);
    --VulkanRenderer2D::GetStats().Allocations;
    --VulkanRenderer2D::GetStats().AllocatedImages;
}

VmaAllocation VulkanAllocator::CreateBuffer(const VkBufferCreateInfo& InBufferCreateInfo, VkBuffer* InBuffer,
                                            VmaMemoryUsage InMemoryUsage) const
{
    VmaAllocationCreateInfo AllocationCreateInfo = {};
    AllocationCreateInfo.usage                   = InMemoryUsage;

    if (InBufferCreateInfo.usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT)  // Staging buffer case
    {
        AllocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        AllocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    VmaAllocation Allocation = VK_NULL_HANDLE;
    const auto result        = vmaCreateBuffer(m_Allocator, &InBufferCreateInfo, &AllocationCreateInfo, InBuffer, &Allocation, nullptr);
    ELS_ASSERT(result == VK_SUCCESS, "Failed to create buffer via VMA!");

    if (InMemoryUsage & VMA_MEMORY_USAGE_GPU_ONLY)
    {
        VmaAllocationInfo AllocationInfo = {};
        vmaGetAllocationInfo(m_Allocator, Allocation, &AllocationInfo);
        VulkanRenderer2D::GetStats().GPUMemoryAllocated += AllocationInfo.size;
    }

    ++VulkanRenderer2D::GetStats().Allocations;
    ++VulkanRenderer2D::GetStats().AllocatedBuffers;
    return Allocation;
}

void VulkanAllocator::DestroyBuffer(VkBuffer& InBuffer, VmaAllocation& InAllocation) const
{
    auto& Context                    = (VulkanContext&)VulkanContext::Get();
    VmaAllocationInfo AllocationInfo = {};
    vmaGetAllocationInfo(m_Allocator, InAllocation, &AllocationInfo);
    if ((Context.GetDevice()->GetMemoryProperties().memoryHeaps[AllocationInfo.memoryType - 1].flags &
         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    {
        VulkanRenderer2D::GetStats().GPUMemoryAllocated -= AllocationInfo.size;
    }

    vmaDestroyBuffer(m_Allocator, InBuffer, InAllocation);
    --VulkanRenderer2D::GetStats().Allocations;
    --VulkanRenderer2D::GetStats().AllocatedBuffers;
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
#if ELS_DEBUG
    LOG_WARN("Before VMA destroying! Remaining data: buffers (%u), images (%u).", VulkanRenderer2D::GetStats().AllocatedBuffers,
             VulkanRenderer2D::GetStats().AllocatedImages);
#endif

    vmaDestroyAllocator(m_Allocator);

#if ELS_DEBUG
    LOG_WARN("VMA destroyed! Remaining data: buffers (%u), images (%u).", VulkanRenderer2D::GetStats().AllocatedBuffers,
             VulkanRenderer2D::GetStats().AllocatedImages);
#endif
}

}  // namespace Eclipse
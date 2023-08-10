#include "GauntletPCH.h"
#include "VulkanAllocator.h"

#include "VulkanUtility.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanTexture.h"

#include "Gauntlet/Renderer/Renderer.h"

namespace Gauntlet
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
    AllocatorCreateInfo.vulkanApiVersion       = GNT_VK_API_VERSION;
    AllocatorCreateInfo.pVulkanFunctions       = &vmaVulkanFunctions;

    const auto result = vmaCreateAllocator(&AllocatorCreateInfo, &m_Allocator);
    GNT_ASSERT(result == VK_SUCCESS, "Failed to create AMD Vulkan Allocator!");
}

VmaAllocation VulkanAllocator::CreateImage(const VkImageCreateInfo& InImageCreateInfo, VkImage* InImage) const
{
    VmaAllocationCreateInfo AllocationCreateInfo = {};
    AllocationCreateInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
    AllocationCreateInfo.flags                   = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    AllocationCreateInfo.requiredFlags           = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VmaAllocation Allocation = {};
    const auto result        = vmaCreateImage(m_Allocator, &InImageCreateInfo, &AllocationCreateInfo, InImage, &Allocation, nullptr);
    GNT_ASSERT(result == VK_SUCCESS, "Failed to create vulkan image via VMA!");

    VmaAllocationInfo AllocationInfo = {};
    QueryAllocationInfo(AllocationInfo, Allocation);

    auto& RendererStats = Renderer::GetStats();
    RendererStats.GPUMemoryAllocated += AllocationInfo.size;

    ++RendererStats.Allocations;
    ++RendererStats.AllocatedImages;
    return Allocation;
}

void VulkanAllocator::DestroyImage(VkImage& InImage, VmaAllocation& InAllocation) const
{
    VmaAllocationInfo AllocationInfo = {};
    QueryAllocationInfo(AllocationInfo, InAllocation);

    auto& RendererStats = Renderer::GetStats();
    RendererStats.GPUMemoryAllocated -= AllocationInfo.size;

    vmaDestroyImage(m_Allocator, InImage, InAllocation);
    --RendererStats.Allocations;
    --RendererStats.AllocatedImages;
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
    GNT_ASSERT(result == VK_SUCCESS, "Failed to create buffer via VMA!");

    auto& RendererStats = Renderer::GetStats();
    if (InMemoryUsage & VMA_MEMORY_USAGE_GPU_ONLY)
    {
        VmaAllocationInfo AllocationInfo = {};
        QueryAllocationInfo(AllocationInfo, Allocation);
        RendererStats.GPUMemoryAllocated += AllocationInfo.size;
    }

    ++RendererStats.Allocations;
    ++RendererStats.AllocatedBuffers;
    return Allocation;
}

void VulkanAllocator::DestroyBuffer(VkBuffer& InBuffer, VmaAllocation& InAllocation) const
{
    auto& Context                    = (VulkanContext&)VulkanContext::Get();
    VmaAllocationInfo AllocationInfo = {};
    QueryAllocationInfo(AllocationInfo, InAllocation);

    auto& RendererStats = Renderer::GetStats();
    if ((Context.GetDevice()->GetMemoryProperties().memoryHeaps[AllocationInfo.memoryType - 1].flags &
         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    {
        RendererStats.GPUMemoryAllocated -= AllocationInfo.size;
    }

    vmaDestroyBuffer(m_Allocator, InBuffer, InAllocation);
    --RendererStats.Allocations;
    --RendererStats.AllocatedBuffers;
}

void VulkanAllocator::QueryAllocationInfo(VmaAllocationInfo& InOutAllocationInfo, const VmaAllocation& InAllocation) const
{
    vmaGetAllocationInfo(m_Allocator, InAllocation, &InOutAllocationInfo);
}

void* VulkanAllocator::Map(VmaAllocation& InAllocation) const
{
    void* Mapped = nullptr;

    const auto result = vmaMapMemory(m_Allocator, InAllocation, &Mapped);
    GNT_ASSERT(result == VK_SUCCESS && Mapped, "Failed to map memory!");

    return Mapped;
}

void VulkanAllocator::Unmap(VmaAllocation& InAllocation) const
{
    vmaUnmapMemory(m_Allocator, InAllocation);
}

void VulkanAllocator::Destroy()
{
    auto& RendererStats = Renderer::GetStats();
#if GNT_DEBUG
    LOG_WARN("Before VMA destroying. Remaining data: buffers (%u), images (%u).", RendererStats.AllocatedBuffers,
             RendererStats.AllocatedImages);
#endif

    vmaDestroyAllocator(m_Allocator);
}

}  // namespace Gauntlet
#include "GauntletPCH.h"
#include "VulkanAllocator.h"

#include "VulkanUtility.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanTexture.h"

#include "Gauntlet/Renderer/Renderer.h"

namespace Gauntlet
{

static bool IsAllocatedOnGPU(const Scoped<VulkanDevice>& device, const VmaAllocationInfo& allocationInfo)
{
    for (uint32_t i = 0; i < device->GetMemoryProperties().memoryTypeCount; ++i)
    {
        if ((device->GetMemoryProperties().memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ==
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT &&
            i == allocationInfo.memoryType)
        {
            return true;
        }

        if (i > allocationInfo.memoryType) return false;
    }

    return false;
}

VulkanAllocator::VulkanAllocator(const VkInstance& instance, const Scoped<VulkanDevice>& device)
{
    // https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/configuration.html#config_Vulkan_functions
    VmaVulkanFunctions vmaVulkanFunctions    = {};
    vmaVulkanFunctions.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;
    vmaVulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;

    VmaAllocatorCreateInfo AllocatorCreateInfo = {};
    AllocatorCreateInfo.instance               = instance;
    AllocatorCreateInfo.device                 = device->GetLogicalDevice();
    AllocatorCreateInfo.physicalDevice         = device->GetPhysicalDevice();
    AllocatorCreateInfo.vulkanApiVersion       = GNT_VK_API_VERSION;
    AllocatorCreateInfo.pVulkanFunctions       = &vmaVulkanFunctions;

    VK_CHECK(vmaCreateAllocator(&AllocatorCreateInfo, &m_Allocator), "Failed to create AMD Vulkan Allocator!");
}

VmaAllocation VulkanAllocator::CreateImage(const VkImageCreateInfo& imageCreateInfo, VkImage* outImage) const
{
    VmaAllocationCreateInfo AllocationCreateInfo = {};
    AllocationCreateInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
    AllocationCreateInfo.flags                   = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    AllocationCreateInfo.requiredFlags           = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VmaAllocation Allocation = {};
    VK_CHECK(vmaCreateImage(m_Allocator, &imageCreateInfo, &AllocationCreateInfo, outImage, &Allocation, nullptr),
             "Failed to create vulkan image via VMA!");

    VmaAllocationInfo AllocationInfo = {};
    QueryAllocationInfo(AllocationInfo, Allocation);

    auto& RendererStats = Renderer::GetStats();
    RendererStats.GPUMemoryAllocated += AllocationInfo.size;

    ++RendererStats.Allocations;
    ++RendererStats.AllocatedImages;
    return Allocation;
}

void VulkanAllocator::DestroyImage(VkImage& image, VmaAllocation& allocation) const
{
    VmaAllocationInfo AllocationInfo = {};
    QueryAllocationInfo(AllocationInfo, allocation);

    auto& RendererStats = Renderer::GetStats();
    RendererStats.GPUMemoryAllocated -= AllocationInfo.size;

    vmaDestroyImage(m_Allocator, image, allocation);
    --RendererStats.Allocations;
    --RendererStats.AllocatedImages;
}

VmaAllocation VulkanAllocator::CreateBuffer(const VkBufferCreateInfo& bufferCreateInfo, VkBuffer* outBuffer,
                                            VmaMemoryUsage memoryUsage) const
{
    VmaAllocationCreateInfo AllocationCreateInfo = {};
    AllocationCreateInfo.usage                   = memoryUsage;

    if (bufferCreateInfo.usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT)  // Staging buffer case
    {
        AllocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        AllocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    VmaAllocation Allocation = VK_NULL_HANDLE;
    VK_CHECK(vmaCreateBuffer(m_Allocator, &bufferCreateInfo, &AllocationCreateInfo, outBuffer, &Allocation, nullptr),
             "Failed to create buffer via VMA!");

    auto& RendererStats = Renderer::GetStats();
    if (memoryUsage & VMA_MEMORY_USAGE_GPU_ONLY)
    {
        VmaAllocationInfo AllocationInfo = {};
        QueryAllocationInfo(AllocationInfo, Allocation);
        RendererStats.GPUMemoryAllocated += AllocationInfo.size;
    }

    ++RendererStats.Allocations;
    ++RendererStats.AllocatedBuffers;
    return Allocation;
}

void VulkanAllocator::DestroyBuffer(VkBuffer& buffer, VmaAllocation& allocation) const
{
    auto& context                    = (VulkanContext&)VulkanContext::Get();
    VmaAllocationInfo AllocationInfo = {};
    QueryAllocationInfo(AllocationInfo, allocation);

    auto& RendererStats = Renderer::GetStats();

    if (IsAllocatedOnGPU(context.GetDevice(), AllocationInfo))
    {
        RendererStats.GPUMemoryAllocated -= AllocationInfo.size;
    }

    vmaDestroyBuffer(m_Allocator, buffer, allocation);
    --RendererStats.Allocations;
    --RendererStats.AllocatedBuffers;
}

void VulkanAllocator::QueryAllocationInfo(VmaAllocationInfo& allocationInfo, const VmaAllocation& allocation) const
{
    vmaGetAllocationInfo(m_Allocator, allocation, &allocationInfo);
}

void* VulkanAllocator::Map(VmaAllocation& allocation) const
{
    void* Mapped = nullptr;
    VK_CHECK(vmaMapMemory(m_Allocator, allocation, &Mapped), "Failed to map memory!");

    return Mapped;
}

void VulkanAllocator::Unmap(VmaAllocation& allocation) const
{
    vmaUnmapMemory(m_Allocator, allocation);
}

void VulkanAllocator::Destroy()
{
    const auto& RendererStats = Renderer::GetStats();
    if (RendererStats.AllocatedBuffers.load() != 0 || RendererStats.AllocatedImages.load() != 0)
        LOG_WARN("Seems like you forgot to destroy something... Remaining data: buffers (%u), images (%u).",
                 RendererStats.AllocatedBuffers.load(), RendererStats.AllocatedImages.load());

    vmaDestroyAllocator(m_Allocator);
}

}  // namespace Gauntlet
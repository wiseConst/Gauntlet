#include "GauntletPCH.h"
#include "VulkanAllocator.h"

#include "VulkanUtility.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanTexture.h"

#include "Gauntlet/Renderer/Renderer.h"

// TODO: Fix VRAM limits

namespace Gauntlet
{

static bool IsAllocatedOnGPU(const Scoped<VulkanDevice>& device, const VmaAllocationInfo& allocationInfo)
{
    for (uint32_t i = 0; i < device->GetMemoryProperties().memoryTypeCount; ++i)
    {
        if ((device->GetMemoryProperties().memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) &&
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

    // Retrieve vram size, allocation will depend on it.
    VkDeviceSize maxVRAMsize = 0;
    for (uint32_t i = 0; i < device->GetMemoryProperties().memoryHeapCount; ++i)
    {
        auto& currentHeap = device->GetMemoryProperties().memoryHeaps[i];
        if ((currentHeap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) == VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            maxVRAMsize = std::max(maxVRAMsize, currentHeap.size);
    }
    GNT_ASSERT(maxVRAMsize > 0, "VRAM Memory 0 GB?");

    m_VRAMSize                                 = maxVRAMsize;
    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.instance               = instance;
    allocatorCreateInfo.device                 = device->GetLogicalDevice();
    allocatorCreateInfo.physicalDevice         = device->GetPhysicalDevice();
    allocatorCreateInfo.vulkanApiVersion       = GNT_VK_API_VERSION;
    allocatorCreateInfo.pVulkanFunctions       = &vmaVulkanFunctions;

    VK_CHECK(vmaCreateAllocator(&allocatorCreateInfo, &m_Allocator), "Failed to create AMD Vulkan Allocator!");
}

VmaAllocation VulkanAllocator::CreateImage(const VkImageCreateInfo& imageCreateInfo, VkImage* outImage) const
{
    VmaAllocationCreateInfo allocationCreateInfo = {};
    // Prefer allocate on GPU
    allocationCreateInfo.usage         = VMA_MEMORY_USAGE_GPU_ONLY;
    allocationCreateInfo.flags         = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    allocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    auto& rendererStats = Renderer::GetStats();
    // Approximate image size
    const VkDeviceSize approximateImageSize = imageCreateInfo.extent.width * imageCreateInfo.extent.height * 4;

    // Limiting VRAM usage up to 80% -> use system RAM
    if (rendererStats.GPUMemoryAllocated + approximateImageSize >= m_VRAMSize * 0.80f)
    {
        allocationCreateInfo.usage         = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        allocationCreateInfo.flags         = 0;
        allocationCreateInfo.requiredFlags = 0;
    }

    VmaAllocation allocation = {};
    VK_CHECK(vmaCreateImage(m_Allocator, &imageCreateInfo, &allocationCreateInfo, outImage, &allocation, nullptr),
             "Failed to create vulkan image via VMA!");

    VmaAllocationInfo allocationInfo = {};
    QueryAllocationInfo(allocationInfo, allocation);

    auto& context = (VulkanContext&)VulkanContext::Get();
    if (IsAllocatedOnGPU(context.GetDevice(), allocationInfo))
    {
        rendererStats.GPUMemoryAllocated += allocationInfo.size;

        //    LOG_WARN("CreateImage: %0.2f MB", rendererStats.GPUMemoryAllocated.load() / 1024.0f / 1024.0f);
    }
    else
        rendererStats.RAMMemoryAllocated += allocationInfo.size;

    ++rendererStats.Allocations;
    ++rendererStats.AllocatedImages;
    return allocation;
}

void VulkanAllocator::DestroyImage(VkImage& image, VmaAllocation& allocation) const
{
    VmaAllocationInfo allocationInfo = {};
    QueryAllocationInfo(allocationInfo, allocation);

    auto& rendererStats = Renderer::GetStats();
    auto& context       = (VulkanContext&)VulkanContext::Get();
    if (IsAllocatedOnGPU(context.GetDevice(), allocationInfo))
        rendererStats.GPUMemoryAllocated -= allocationInfo.size;
    else
        rendererStats.RAMMemoryAllocated -= allocationInfo.size;

    vmaDestroyImage(m_Allocator, image, allocation);
    --rendererStats.Allocations;
    --rendererStats.AllocatedImages;
}

VmaAllocation VulkanAllocator::CreateBuffer(const VkBufferCreateInfo& bufferCreateInfo, VkBuffer* outBuffer,
                                            VmaMemoryUsage memoryUsage) const
{
    VmaAllocationCreateInfo allocationCreateInfo = {};
    allocationCreateInfo.usage                   = memoryUsage;
    auto& rendererStats                          = Renderer::GetStats();

    if (rendererStats.GPUMemoryAllocated + bufferCreateInfo.size >= m_VRAMSize * 0.80f && memoryUsage == VMA_MEMORY_USAGE_GPU_ONLY)
    {
        allocationCreateInfo.usage         = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        allocationCreateInfo.flags         = 0;
        allocationCreateInfo.requiredFlags = 0;
    }
    else if (bufferCreateInfo.usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT)  // Staging buffer case
    {
        allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    VmaAllocation allocation = VK_NULL_HANDLE;
    VK_CHECK(vmaCreateBuffer(m_Allocator, &bufferCreateInfo, &allocationCreateInfo, outBuffer, &allocation, nullptr),
             "Failed to create buffer via VMA!");

    VmaAllocationInfo allocationInfo = {};
    QueryAllocationInfo(allocationInfo, allocation);

    auto& context = (VulkanContext&)VulkanContext::Get();
    if (IsAllocatedOnGPU(context.GetDevice(), allocationInfo))
    {
        rendererStats.GPUMemoryAllocated += allocationInfo.size;
        //  LOG_WARN("CreateBuffer: %0.2f MB", rendererStats.GPUMemoryAllocated.load() / 1024.0f / 1024.0f);
    }
    else
        rendererStats.RAMMemoryAllocated += allocationInfo.size;

    ++rendererStats.Allocations;
    ++rendererStats.AllocatedBuffers;
    return allocation;
}

void VulkanAllocator::DestroyBuffer(VkBuffer& buffer, VmaAllocation& allocation) const
{
    auto& context                    = (VulkanContext&)VulkanContext::Get();
    VmaAllocationInfo allocationInfo = {};
    QueryAllocationInfo(allocationInfo, allocation);

    auto& rendererStats = Renderer::GetStats();
    if (IsAllocatedOnGPU(context.GetDevice(), allocationInfo))
        rendererStats.GPUMemoryAllocated -= allocationInfo.size;
    else
        rendererStats.RAMMemoryAllocated -= allocationInfo.size;

    vmaDestroyBuffer(m_Allocator, buffer, allocation);
    --rendererStats.Allocations;
    --rendererStats.AllocatedBuffers;
}

void VulkanAllocator::QueryAllocationInfo(VmaAllocationInfo& allocationInfo, const VmaAllocation& allocation) const
{
    vmaGetAllocationInfo(m_Allocator, allocation, &allocationInfo);
}

void* VulkanAllocator::Map(VmaAllocation& allocation) const
{
    void* mapped = nullptr;
    VK_CHECK(vmaMapMemory(m_Allocator, allocation, &mapped), "Failed to map memory!");

    return mapped;
}

void VulkanAllocator::Unmap(VmaAllocation& allocation) const
{
    vmaUnmapMemory(m_Allocator, allocation);
}

void VulkanAllocator::Destroy()
{
    const auto& rendererStats = Renderer::GetStats();
    if (rendererStats.AllocatedBuffers.load() != 0 || rendererStats.AllocatedImages.load() != 0)
        LOG_WARN("Seems like you forgot to destroy something... Remaining data: buffers (%u), images (%u).",
                 rendererStats.AllocatedBuffers.load(), rendererStats.AllocatedImages.load());

    vmaDestroyAllocator(m_Allocator);
}

}  // namespace Gauntlet
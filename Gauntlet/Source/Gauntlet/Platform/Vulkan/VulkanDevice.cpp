#include "GauntletPCH.h"
#include "VulkanDevice.h"

#include "SwapchainSupportDetails.h"
#include "VulkanUtility.h"

namespace Gauntlet
{

static const char* GetVendorNameCString(uint32_t vendorID)
{
    switch (vendorID)
    {
        case 0x1002:
        {
            return "AMD";
        }
        case 0x1010:
        {
            return "ImgTec";
        }
        case 0x10DE:
        {
            return "NVIDIA";
        }
        case 0x13B5:
        {
            return "ARM";
        }
        case 0x5143:
        {
            return "Qualcomm";
        }
        case 0x8086:
        {
            return "INTEL";
        }
    }

    GNT_ASSERT(false, "Unknown vendor!");
    return nullptr;
}

static const char* GetDeviceTypeCString(VkPhysicalDeviceType deviceType)
{
    switch (deviceType)
    {
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
        {
            return "OTHER";
        }
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        {
            return "INTEGRATED_GPU";
        }
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        {
            return "DISCRETE_GPU";
        }
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        {
            return "VIRTUAL_GPU";
        }
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
        {
            return "CPU";
        }
    }

    GNT_ASSERT(false, "Unknown device type!");
    return nullptr;
}

VulkanDevice::VulkanDevice(const VkInstance& instance, const VkSurfaceKHR& surface)
{
    PickPhysicalDevice(instance, surface);
    CreateLogicalDevice();
}

void VulkanDevice::PickPhysicalDevice(const VkInstance& instance, const VkSurfaceKHR& surface)
{
    uint32_t GPUsCount = 0;
    {
        const auto result = vkEnumeratePhysicalDevices(instance, &GPUsCount, nullptr);
        GNT_ASSERT(result == VK_SUCCESS && GPUsCount > 0, "Failed to retrieve gpu count.");
    }

    std::vector<VkPhysicalDevice> PhysicalDevices(GPUsCount);
    {
        const auto result = vkEnumeratePhysicalDevices(instance, &GPUsCount, PhysicalDevices.data());
        GNT_ASSERT(result == VK_SUCCESS && GPUsCount > 0, "Failed to retrieve gpu count.");
    }

#if LOG_VULKAN_INFO
    LOG_INFO("Available GPUs: %u", GPUsCount);
#endif

    std::multimap<uint32_t, GPUInfo> Candidates;
    for (const auto& PhysicalDevice : PhysicalDevices)
    {
        GPUInfo CurrentGPU        = {};
        CurrentGPU.PhysicalDevice = PhysicalDevice;

        const uint32_t Score = RateDeviceSuitability(CurrentGPU, surface);
        Candidates.insert(std::make_pair(Score, CurrentGPU));
    }

    if (Candidates.rbegin()->first > 0)
    {
        m_GPUInfo = Candidates.rbegin()->second;
    }
    GNT_ASSERT(m_GPUInfo.PhysicalDevice, "Failed to find suitable GPU");
    LOG_INFO("Renderer: %s", m_GPUInfo.GPUProperties.deviceName);
    LOG_INFO(" Vendor: %s", GetVendorNameCString(m_GPUInfo.GPUProperties.vendorID));
    LOG_INFO(" Driver Version: %s %s", m_GPUInfo.GPUDriverProperties.driverName, m_GPUInfo.GPUDriverProperties.driverInfo);
    LOG_INFO(" Using Vulkan API Version: %u.%u.%u", VK_API_VERSION_MAJOR(m_GPUInfo.GPUProperties.apiVersion),
             VK_API_VERSION_MINOR(m_GPUInfo.GPUProperties.apiVersion), VK_API_VERSION_PATCH(m_GPUInfo.GPUProperties.apiVersion));
}

void VulkanDevice::CreateLogicalDevice()
{
    const float QueuePriority = 1.0f;  // [0.0, 1.0]

    std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos = {};
    std::set<uint32_t> UniqueQueueFamilies{m_GPUInfo.QueueFamilyIndices.GraphicsFamily, m_GPUInfo.QueueFamilyIndices.PresentFamily,
                                           m_GPUInfo.QueueFamilyIndices.TransferFamily};

    for (auto QueueFamily : UniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo QueueCreateInfo = {};
        QueueCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        QueueCreateInfo.pQueuePriorities        = &QueuePriority;
        QueueCreateInfo.queueCount              = 1;
        QueueCreateInfo.queueFamilyIndex        = QueueFamily;
        QueueCreateInfos.push_back(QueueCreateInfo);
    }

    VkDeviceCreateInfo deviceCI   = {};
    deviceCI.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCI.pQueueCreateInfos    = QueueCreateInfos.data();
    deviceCI.queueCreateInfoCount = static_cast<uint32_t>(QueueCreateInfos.size());

    VkPhysicalDeviceVulkan12Features Vulkan12Features          = {};
    Vulkan12Features.sType                                     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    Vulkan12Features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    Vulkan12Features.descriptorBindingPartiallyBound           = VK_TRUE;
    Vulkan12Features.pNext                                     = VK_NULL_HANDLE;

    deviceCI.pNext = &Vulkan12Features;

    VkPhysicalDeviceFeatures PhysicalDeviceFeatures = {};
    PhysicalDeviceFeatures.samplerAnisotropy        = VK_TRUE;
    PhysicalDeviceFeatures.fillModeNonSolid         = VK_TRUE;

    deviceCI.pEnabledFeatures        = &PhysicalDeviceFeatures;
    deviceCI.enabledExtensionCount   = static_cast<uint32_t>(DeviceExtensions.size());
    deviceCI.ppEnabledExtensionNames = DeviceExtensions.data();

    {
        const auto result = vkCreateDevice(m_GPUInfo.PhysicalDevice, &deviceCI, nullptr, &m_GPUInfo.LogicalDevice);
        GNT_ASSERT(result == VK_SUCCESS, "Failed to create vulkan logical device && queues!");
    }
    volkLoadDevice(m_GPUInfo.LogicalDevice);

    vkGetDeviceQueue(m_GPUInfo.LogicalDevice, m_GPUInfo.QueueFamilyIndices.GraphicsFamily, 0, &m_GPUInfo.GraphicsQueue);
    vkGetDeviceQueue(m_GPUInfo.LogicalDevice, m_GPUInfo.QueueFamilyIndices.PresentFamily, 0, &m_GPUInfo.PresentQueue);
    vkGetDeviceQueue(m_GPUInfo.LogicalDevice, m_GPUInfo.QueueFamilyIndices.TransferFamily, 0, &m_GPUInfo.TransferQueue);
    vkGetDeviceQueue(m_GPUInfo.LogicalDevice, m_GPUInfo.QueueFamilyIndices.ComputeFamily, 0, &m_GPUInfo.ComputeQueue);

    GNT_ASSERT(m_GPUInfo.GraphicsQueue && m_GPUInfo.PresentQueue && m_GPUInfo.TransferQueue && m_GPUInfo.ComputeQueue,
               "Failed to retrieve queue handles!");
}

uint32_t VulkanDevice::RateDeviceSuitability(GPUInfo& gpuInfo, const VkSurfaceKHR& surface)
{
    if (!IsDeviceSuitable(gpuInfo, surface))
    {
        LOG_WARN("GPU above is not suitable");
        return 0;
    }

    // Discrete GPUs have a significant performance advantage
    uint32_t score = 0;
    switch (gpuInfo.GPUProperties.deviceType)
    {
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
        {
            score += 50;
            break;
        }
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        {
            score += 800;
            break;
        }
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        {
            score += 1500;
            break;
        }
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        {
            score += 250;
            break;
        }
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
        {
            score += 400;
            break;
        }
    }

    // Maximum possible size of textures affects graphics quality
    score += gpuInfo.GPUProperties.limits.maxImageDimension2D;

    score += gpuInfo.GPUProperties.limits.maxViewports;

    // Maximum size of fast(-accessible) uniform data in shaders.
    score += gpuInfo.GPUProperties.limits.maxPushConstantsSize;

    return score;
}

bool VulkanDevice::IsDeviceSuitable(GPUInfo& gpuInfo, const VkSurfaceKHR& surface)
{
    // Query GPU properties(device name, limits, etc..)
    vkGetPhysicalDeviceProperties(gpuInfo.PhysicalDevice, &gpuInfo.GPUProperties);

    // Query GPU features(geometry shader support, multiViewport support)
    vkGetPhysicalDeviceFeatures(gpuInfo.PhysicalDevice, &gpuInfo.GPUFeatures);

    // Query GPU memory properties
    vkGetPhysicalDeviceMemoryProperties(gpuInfo.PhysicalDevice, &gpuInfo.GPUMemoryProperties);

    gpuInfo.GPUDriverProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;

    VkPhysicalDeviceProperties2 GPUProperties2 = {};
    GPUProperties2.sType                       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    GPUProperties2.pNext                       = &gpuInfo.GPUDriverProperties;
    vkGetPhysicalDeviceProperties2(gpuInfo.PhysicalDevice, &GPUProperties2);

#if LOG_VULKAN_INFO
    LOG_INFO("GPU info:");
    LOG_TRACE(" Renderer: %s", gpuInfo.GPUProperties.deviceName);
    LOG_TRACE(" Vendor: %s/%u", GetVendorNameCString(gpuInfo.GPUProperties.vendorID), gpuInfo.GPUProperties.vendorID);
    LOG_TRACE(" %s %s", gpuInfo.GPUDriverProperties.driverName, gpuInfo.GPUDriverProperties.driverInfo);
    LOG_TRACE(" API Version %u.%u.%u", VK_API_VERSION_MAJOR(gpuInfo.GPUProperties.apiVersion),
              VK_API_VERSION_MINOR(gpuInfo.GPUProperties.apiVersion), VK_API_VERSION_PATCH(gpuInfo.GPUProperties.apiVersion));
    LOG_INFO(" Device Type: %s", GetDeviceTypeCString(gpuInfo.GPUProperties.deviceType));
    LOG_INFO(" Max Push Constants Size(depends on gpu): %u", gpuInfo.GPUProperties.limits.maxPushConstantsSize);
    LOG_INFO(" Max Sampler Anisotropy: %0.0f", gpuInfo.GPUProperties.limits.maxSamplerAnisotropy);
    LOG_INFO(" Max Sampler Allocations: %u", gpuInfo.GPUProperties.limits.maxSamplerAllocationCount);
    LOG_INFO(" Min Uniform Buffer Offset Alignment: %u", gpuInfo.GPUProperties.limits.minUniformBufferOffsetAlignment);
    LOG_INFO(" Min Memory Map Alignment: %u", gpuInfo.GPUProperties.limits.minMemoryMapAlignment);
    LOG_INFO(" Min Storage Buffer Offset Alignment: %u", gpuInfo.GPUProperties.limits.minStorageBufferOffsetAlignment);
    LOG_INFO(" Max Memory Allocations: %u", gpuInfo.GPUProperties.limits.maxMemoryAllocationCount);

    LOG_INFO(" Memory types: %u", gpuInfo.GPUMemoryProperties.memoryTypeCount);
    for (uint32_t i = 0; i < gpuInfo.GPUMemoryProperties.memoryTypeCount; ++i)
    {
        LOG_INFO("  HeapIndex: %u", gpuInfo.GPUMemoryProperties.memoryTypes[i].heapIndex);

        if (gpuInfo.GPUMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            LOG_TRACE("    DEVICE_LOCAL_BIT");

        if (gpuInfo.GPUMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
            LOG_TRACE("    HOST_VISIBLE_BIT");

        if (gpuInfo.GPUMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
            LOG_TRACE("    HOST_COHERENT_BIT");

        if (gpuInfo.GPUMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) LOG_TRACE("    HOST_CACHED_BIT");

        if (gpuInfo.GPUMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
            LOG_TRACE("    LAZILY_ALLOCATED_BIT");

        if (gpuInfo.GPUMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT) LOG_TRACE("    PROTECTED_BIT");

        if (gpuInfo.GPUMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD)
            LOG_INFO("    DEVICE_COHERENT_BIT_AMD");

        if (gpuInfo.GPUMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD)
            LOG_TRACE("    UNCACHED_BIT_AMD");

        if (gpuInfo.GPUMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV)
            LOG_TRACE("    RDMA_CAPABLE_BIT_NV");
    }

    LOG_INFO(" Memory heaps: %u", gpuInfo.GPUMemoryProperties.memoryHeapCount);
    for (uint32_t i = 0; i < gpuInfo.GPUMemoryProperties.memoryHeapCount; ++i)
    {
        LOG_TRACE("  Size: %u MB", gpuInfo.GPUMemoryProperties.memoryHeaps[i].size / 1024 / 1024);

        if (gpuInfo.GPUMemoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) LOG_TRACE("    HEAP_DEVICE_LOCAL_BIT");

        if (gpuInfo.GPUMemoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT) LOG_TRACE("    HEAP_MULTI_INSTANCE_BIT");

        if ((gpuInfo.GPUMemoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) == 0 &&
            (gpuInfo.GPUMemoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT) == 0)
            LOG_TRACE("    HEAP_SYSTEM_RAM");
    }
#endif

    QueueFamilyIndices Indices = QueueFamilyIndices::FindQueueFamilyIndices(surface, gpuInfo.PhysicalDevice);
    gpuInfo.QueueFamilyIndices = Indices;

    bool bIsSwapchainAdequate               = false;
    const bool bIsDeviceExtensionsSupported = CheckDeviceExtensionSupport(gpuInfo.PhysicalDevice);
    if (bIsDeviceExtensionsSupported)
    {
        SwapchainSupportDetails Details = SwapchainSupportDetails::QuerySwapchainSupportDetails(gpuInfo.PhysicalDevice, surface);
        bIsSwapchainAdequate            = !Details.ImageFormats.empty() && !Details.PresentModes.empty();
    }

    GNT_ASSERT(gpuInfo.GPUProperties.limits.maxSamplerAnisotropy > 0, "GPU has not valid Max Sampler Anisotropy!");
    return gpuInfo.GPUFeatures.samplerAnisotropy && gpuInfo.GPUFeatures.geometryShader && Indices.IsComplete() &&
           bIsDeviceExtensionsSupported && bIsSwapchainAdequate;
}

bool VulkanDevice::CheckDeviceExtensionSupport(const VkPhysicalDevice& physicalDevice)
{
    uint32_t ExtensionCount{0};
    VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &ExtensionCount, nullptr),
             "Failed to retrieve number of device extensions.");
    GNT_ASSERT(ExtensionCount > 0, "Device extensions <= 0!");

    std::vector<VkExtensionProperties> AvailableExtensions(ExtensionCount);

    VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &ExtensionCount, AvailableExtensions.data()),
             "Failed to retrieve device extensions.");

    std::set<std::string> RequiredExtensions(DeviceExtensions.begin(), DeviceExtensions.end());
    for (const auto& Ext : AvailableExtensions)
    {
        if (RequiredExtensions.empty()) return true;

        RequiredExtensions.erase(Ext.extensionName);
    }

    return RequiredExtensions.empty();
}

void VulkanDevice::Destroy()
{
    vkDestroyDevice(m_GPUInfo.LogicalDevice, nullptr);
}

}  // namespace Gauntlet
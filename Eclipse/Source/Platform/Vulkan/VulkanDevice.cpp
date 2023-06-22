#include "EclipsePCH.h"
#include "VulkanDevice.h"

#include "SwapchainSupportDetails.h"
#include "VulkanUtility.h"

namespace Eclipse
{
VulkanDevice::VulkanDevice(const VkInstance& InInstance, const VkSurfaceKHR& InSurface)
{
    PickPhysicalDevice(InInstance, InSurface);
    CreateLogicalDevice(InSurface);
}

void VulkanDevice::Destroy()
{
    vkDestroyDevice(m_LogicalDevice, nullptr);
}

void VulkanDevice::PickPhysicalDevice(const VkInstance& InInstance, const VkSurfaceKHR& InSurface)
{
    uint32_t GPUsCount = 0;
    {
        const auto result = vkEnumeratePhysicalDevices(InInstance, &GPUsCount, nullptr);
        ELS_ASSERT(result == VK_SUCCESS && GPUsCount > 0, "Failed to retrieve gpu count.")
    }

    std::vector<VkPhysicalDevice> PhysicalDevices(GPUsCount);
    {
        const auto result = vkEnumeratePhysicalDevices(InInstance, &GPUsCount, PhysicalDevices.data());
        ELS_ASSERT(result == VK_SUCCESS && GPUsCount > 0, "Failed to retrieve gpu count.")
    }

    LOG_INFO("Available GPUs: %u", GPUsCount);

    std::multimap<uint32_t, VkPhysicalDevice> Candidates;
    for (const auto& PhysDevice : PhysicalDevices)
    {
        const uint32_t Score = RateDeviceSuitability(PhysDevice, InSurface);
        Candidates.insert(std::make_pair(Score, PhysDevice));
    }

    if (Candidates.rbegin()->first > 0)
    {
        m_PhysicalDevice = Candidates.rbegin()->second;
    }
    ELS_ASSERT(m_PhysicalDevice, "Failed to find suitable GPU");

    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_GPUProperties);
    vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &m_GPUFeatures);
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_GPUMemoryProperties);
    LOG_INFO("Chosen GPU: %s", m_GPUProperties.deviceName);
}

void VulkanDevice::CreateLogicalDevice(const VkSurfaceKHR& InSurface)
{
    const float QueuePriority = 1.0f;  // [0.0,1.0]
    m_QueueFamilyIndices = QueueFamilyIndices::FindQueueFamilyIndices(InSurface, m_PhysicalDevice);

    std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos = {};
    std::set<uint32_t> UniqueQueueFamilies{m_QueueFamilyIndices.GetGraphicsFamily(), m_QueueFamilyIndices.GetPresentFamily(),
                                           m_QueueFamilyIndices.GetTransferFamily()};

    for (auto QueueFamily : UniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCI = {};
        queueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCI.pQueuePriorities = &QueuePriority;
        queueCI.queueCount = 1;
        queueCI.queueFamilyIndex = QueueFamily;
        queueCI.pNext = nullptr;
        QueueCreateInfos.push_back(queueCI);
    }

    VkDeviceCreateInfo deviceCI = {};
    deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCI.pQueueCreateInfos = QueueCreateInfos.data();
    deviceCI.queueCreateInfoCount = static_cast<uint32_t>(QueueCreateInfos.size());
    deviceCI.pNext = nullptr;

    VkPhysicalDeviceFeatures features = {};
    features.samplerAnisotropy = VK_TRUE;

    deviceCI.pEnabledFeatures = &features;
    // deviceCI.pEnabledFeatures = &m_GPUFeatures;  // Specifying features(if some of them available) that we want to use.
    deviceCI.enabledExtensionCount = static_cast<uint32_t>(DeviceExtensions.size());
    deviceCI.ppEnabledExtensionNames = DeviceExtensions.data();

    // Deprecated
    deviceCI.enabledLayerCount = 0;
    deviceCI.ppEnabledLayerNames = nullptr;

    {
        const auto result = vkCreateDevice(m_PhysicalDevice, &deviceCI, nullptr, &m_LogicalDevice);
        ELS_ASSERT(result == VK_SUCCESS, "Failed to create logical device && queues!")
    }
    volkLoadDevice(m_LogicalDevice);

    vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.GetGraphicsFamily(), 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.GetPresentFamily(), 0, &m_PresentQueue);
    vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.GetTransferFamily(), 0, &m_TransferQueue);

    /* BY VOLK IMPLEMENTATION
       1)  For applications that use just one VkDevice object, load device
           related Vulkan entrypoints directly from the driver with this function:
                void volkLoadDevice(VkDevice device);

       2) For applications that use multiple VkDevice objects,
          load device related Vulkan entrypoints into a table:
             void volkLoadDeviceTable(struct VolkDeviceTable * table, VkDevice device);
     */
}

bool VulkanDevice::IsDeviceSuitable(const VkPhysicalDevice& InPhysicalDevice, const VkSurfaceKHR& InSurface)
{
    // Query GPU properties(device name, limits, etc..)
    vkGetPhysicalDeviceProperties(InPhysicalDevice, &m_GPUProperties);

    // Query GPU features(geometry shader support, multiViewport support)
    vkGetPhysicalDeviceFeatures(InPhysicalDevice, &m_GPUFeatures);

    // Query GPU memory properties
    vkGetPhysicalDeviceMemoryProperties(InPhysicalDevice, &m_GPUMemoryProperties);

    VkPhysicalDeviceDriverProperties driverProps = {};
    driverProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;

    VkPhysicalDeviceProperties2 gpuProps = {};
    gpuProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    gpuProps.pNext = &driverProps;
    vkGetPhysicalDeviceProperties2(InPhysicalDevice, &gpuProps);

#if LOG_VULKAN_INFO
    LOG_INFO("GPU info:");
    LOG_TRACE(" Renderer: %s", m_GPUProperties.deviceName);
    LOG_TRACE(" Vendor: %s/%u", GetVendorNameCString(m_GPUProperties.vendorID), m_GPUProperties.vendorID);
    LOG_TRACE(" %s %s", driverProps.driverName, driverProps.driverInfo);
    LOG_TRACE(" API Version %u.%u.%u", VK_API_VERSION_MAJOR(m_GPUProperties.apiVersion), VK_API_VERSION_MINOR(m_GPUProperties.apiVersion),
              VK_API_VERSION_PATCH(m_GPUProperties.apiVersion));
    LOG_INFO(" Device Type: %s", GetDeviceTypeCString(m_GPUProperties.deviceType));
    LOG_INFO(" Max Push Constants Size(depends on gpu): %u", m_GPUProperties.limits.maxPushConstantsSize);
    LOG_INFO(" Max memory allocations: %u", m_GPUProperties.limits.maxMemoryAllocationCount);
    LOG_INFO(" Max sampler allocations: %u", m_GPUProperties.limits.maxSamplerAllocationCount);

    LOG_INFO(" Memory types: %u", m_GPUMemoryProperties.memoryTypeCount);
    for (uint32_t i = 0; i < m_GPUMemoryProperties.memoryTypeCount; ++i)
    {
        LOG_INFO("  HeapIndex: %u", m_GPUMemoryProperties.memoryTypes[i].heapIndex);

        if (m_GPUMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) LOG_INFO("    DEVICE_LOCAL_BIT");

        if (m_GPUMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) LOG_INFO("    HOST_VISIBLE_BIT");

        if (m_GPUMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) LOG_INFO("    HOST_COHERENT_BIT");

        if (m_GPUMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) LOG_INFO("    HOST_CACHED_BIT");

        if (m_GPUMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
            LOG_INFO("    LAZILY_ALLOCATED_BIT");

        if (m_GPUMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT) LOG_INFO("    PROTECTED_BIT");

        if (m_GPUMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD)
            LOG_INFO("    DEVICE_COHERENT_BIT_AMD");

        if (m_GPUMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD)
            LOG_INFO("    UNCACHED_BIT_AMD");

        if (m_GPUMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV)
            LOG_INFO("    RDMA_CAPABLE_BIT_NV");
    }

    LOG_INFO(" Memory heaps: %u", m_GPUMemoryProperties.memoryHeapCount);
    for (uint32_t i = 0; i < m_GPUMemoryProperties.memoryHeapCount; ++i)
    {
        LOG_INFO("  Size: %u MB", m_GPUMemoryProperties.memoryHeaps[i].size / 1024 / 1024);

        if (m_GPUMemoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) LOG_INFO("    HEAP_DEVICE_LOCAL_BIT");

        if (m_GPUMemoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT) LOG_INFO("    HEAP_MULTI_INSTANCE_BIT");
    }
#endif

    QueueFamilyIndices indices = QueueFamilyIndices::FindQueueFamilyIndices(InSurface, InPhysicalDevice);

    bool bIsSwapchainAdequate = false;
    const bool bIsDeviceExtensionsSupported = CheckDeviceExtensionSupport(InPhysicalDevice);
    if (bIsDeviceExtensionsSupported)
    {
        SwapchainSupportDetails Details = SwapchainSupportDetails::QuerySwapchainSupportDetails(InPhysicalDevice, InSurface);
        bIsSwapchainAdequate = !Details.ImageFormats.empty() && !Details.PresentModes.empty();
    }

    return m_GPUFeatures.samplerAnisotropy && m_GPUFeatures.geometryShader && indices.IsComplete() && bIsDeviceExtensionsSupported &&
           bIsSwapchainAdequate;
}

const char* VulkanDevice::GetVendorNameCString(uint32_t vendorID)
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

    ELS_ASSERT(false, "Unknown vendor!");
    return nullptr;
}

const char* VulkanDevice::GetDeviceTypeCString(VkPhysicalDeviceType deviceType)
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

    ELS_ASSERT(false, "Unknown device type!");
    return nullptr;
}

uint32_t VulkanDevice::RateDeviceSuitability(const VkPhysicalDevice& InPhysicalDevice, const VkSurfaceKHR& InSurface)
{
    if (!IsDeviceSuitable(InPhysicalDevice, InSurface))
    {
        LOG_WARN("GPU above is not suitable");
        return 0;
    }

    uint32_t score = 0;
    // Discrete GPUs have a significant performance advantage
    switch (m_GPUProperties.deviceType)
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
    score += m_GPUProperties.limits.maxImageDimension2D;

    score += m_GPUProperties.limits.maxViewports;

    // Maximum size of fast(-accessible) uniform data in shaders.
    score += m_GPUProperties.limits.maxPushConstantsSize;

    return score;
}

bool VulkanDevice::CheckDeviceExtensionSupport(const VkPhysicalDevice& InPhysicalDevice)
{
    uint32_t ExtensionCount{0};
    {
        const auto result = vkEnumerateDeviceExtensionProperties(InPhysicalDevice, nullptr, &ExtensionCount, nullptr);
        ELS_ASSERT(result == VK_SUCCESS && ExtensionCount != 0, "Failed to retrieve number of device extensions.");
    }

    std::vector<VkExtensionProperties> AvailableExtensions(ExtensionCount);
    {
        const auto result = vkEnumerateDeviceExtensionProperties(InPhysicalDevice, nullptr, &ExtensionCount, AvailableExtensions.data());
        ELS_ASSERT(result == VK_SUCCESS && ExtensionCount != 0, "Failed to retrieve device extensions.");
    }

    std::set<std::string> RequiredExtensions(DeviceExtensions.begin(), DeviceExtensions.end());
    for (const auto& Ext : AvailableExtensions)
    {
        RequiredExtensions.erase(Ext.extensionName);
    }

    return RequiredExtensions.empty();
}

}  // namespace Eclipse
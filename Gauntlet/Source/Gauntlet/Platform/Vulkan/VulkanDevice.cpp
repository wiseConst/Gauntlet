#include "GauntletPCH.h"
#include "VulkanDevice.h"

#include "SwapchainSupportDetails.h"
#include "VulkanUtility.h"
#include "VulkanCommandBuffer.h"

namespace Gauntlet
{

static const char* GetVendorNameCString(uint32_t vendorID)
{
    switch (vendorID)
    {
        case 0x1002: return "AMD";
        case 0x1010: return "ImgTec";
        case 0x10DE: return "NVIDIA";
        case 0x13B5: return "ARM";
        case 0x5143: return "Qualcomm";
        case 0x8086: return "INTEL";
    }

    GNT_ASSERT(false, "Unknown vendor!");
    return "None";
}

static const char* GetDeviceTypeCString(VkPhysicalDeviceType deviceType)
{
    switch (deviceType)
    {
        case VK_PHYSICAL_DEVICE_TYPE_OTHER: return "OTHER";
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "INTEGRATED_GPU";
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return "DISCRETE_GPU";
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return "VIRTUAL_GPU";
        case VK_PHYSICAL_DEVICE_TYPE_CPU: return "CPU";
    }

    GNT_ASSERT(false, "Unknown device type!");
    return nullptr;
}

VulkanDevice::VulkanDevice(const VkInstance& instance, const VkSurfaceKHR& surface)
{
    PickPhysicalDevice(instance, surface);
    CreateLogicalDevice();
    CreateCommandPools();
}

void VulkanDevice::CreateCommandPools()
{
    VkCommandPoolCreateInfo commandPoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};

    // Graphics command pool
    {
        commandPoolCreateInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolCreateInfo.queueFamilyIndex = m_GPUInfo.QueueFamilyIndices.GraphicsFamily;
        VK_CHECK(vkCreateCommandPool(m_GPUInfo.LogicalDevice, &commandPoolCreateInfo, VK_NULL_HANDLE, &m_GPUInfo.GraphicsCommandPool),
                 "Failed to create graphics command pool!");
    }

    // Async-Compute(if possible) command pool
    {
        commandPoolCreateInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolCreateInfo.queueFamilyIndex = m_GPUInfo.QueueFamilyIndices.ComputeFamily;
        VK_CHECK(vkCreateCommandPool(m_GPUInfo.LogicalDevice, &commandPoolCreateInfo, VK_NULL_HANDLE, &m_GPUInfo.ComputeCommandPool),
                 "Failed to create (async)compute command pool!");
    }

    // Dedicated transfer command pool
    {
        commandPoolCreateInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        commandPoolCreateInfo.queueFamilyIndex = m_GPUInfo.QueueFamilyIndices.TransferFamily;
        VK_CHECK(vkCreateCommandPool(m_GPUInfo.LogicalDevice, &commandPoolCreateInfo, VK_NULL_HANDLE, &m_GPUInfo.TransferCommandPool),
                 "Failed to create (dedicated)transfer command pool!");
    }
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

    if (!GRAPHICS_PREFER_INTEGRATED_GPU)
    {
        m_GPUInfo = Candidates.rbegin()->second;
    }
    else
    {
        for (auto& candidate : Candidates)
        {
            m_GPUInfo = candidate.second;
            if (candidate.second.GPUProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) break;
        }
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
                                           m_GPUInfo.QueueFamilyIndices.TransferFamily, m_GPUInfo.QueueFamilyIndices.ComputeFamily};

    for (auto QueueFamily : UniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo QueueCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        QueueCreateInfo.pQueuePriorities        = &QueuePriority;
        QueueCreateInfo.queueCount              = 1;
        QueueCreateInfo.queueFamilyIndex        = QueueFamily;
        QueueCreateInfos.push_back(QueueCreateInfo);
    }

    VkDeviceCreateInfo deviceCI   = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceCI.pQueueCreateInfos    = QueueCreateInfos.data();
    deviceCI.queueCreateInfoCount = static_cast<uint32_t>(QueueCreateInfos.size());

    // Useful vulkan 1.2 features (bindless)
    VkPhysicalDeviceVulkan12Features vulkan12Features              = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    vulkan12Features.shaderSampledImageArrayNonUniformIndexing     = VK_TRUE;
    vulkan12Features.descriptorBindingPartiallyBound               = VK_TRUE;
    vulkan12Features.descriptorBindingSampledImageUpdateAfterBind  = VK_TRUE;
    vulkan12Features.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
    vulkan12Features.descriptorBindingStorageImageUpdateAfterBind  = VK_TRUE;
    vulkan12Features.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
    vulkan12Features.scalarBlockLayout                             = VK_TRUE;

    deviceCI.pNext = &vulkan12Features;  // chaining extensions
    void** ppNext  = &vulkan12Features.pNext;

#if !RENDERDOC_DEBUG
    // Useful pipeline features that can be changed in real-time(for instance, polygon mode, primitive topology, etc..)
    VkPhysicalDeviceExtendedDynamicState3FeaturesEXT extendedDynamicState3FeaturesEXT = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT};
    extendedDynamicState3FeaturesEXT.extendedDynamicState3PolygonMode = VK_TRUE;

    *ppNext = &extendedDynamicState3FeaturesEXT;
    ppNext  = &extendedDynamicState3FeaturesEXT.pNext;
#endif

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES};
    dynamicRenderingFeatures.dynamicRendering                            = VK_TRUE;

    *ppNext = &dynamicRenderingFeatures;
    ppNext  = &dynamicRenderingFeatures.pNext;

#if VK_RTX
    vulkan12Features.bufferDeviceAddress = VK_TRUE;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
    enabledRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;

    *ppNext = &enabledRayTracingPipelineFeatures;
    ppNext  = &enabledRayTracingPipelineFeatures.pNext;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
    enabledAccelerationStructureFeatures.accelerationStructure = VK_TRUE;

    *ppNext = &enabledAccelerationStructureFeatures;
    ppNext  = &enabledAccelerationStructureFeatures.pNext;
#endif

#if VK_MESH_SHADING
    VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeaturesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT};
    meshShaderFeaturesEXT.meshShaderQueries                     = VK_TRUE;
    meshShaderFeaturesEXT.meshShader                            = VK_TRUE;
    meshShaderFeaturesEXT.taskShader                            = VK_TRUE;

    *ppNext = &meshShaderFeaturesEXT;
    ppNext  = &meshShaderFeaturesEXT.pNext;
#endif

    // Required gpu features
    VkPhysicalDeviceFeatures PhysicalDeviceFeatures = {};
    PhysicalDeviceFeatures.samplerAnisotropy        = VK_TRUE;
    PhysicalDeviceFeatures.fillModeNonSolid         = VK_TRUE;
    PhysicalDeviceFeatures.pipelineStatisticsQuery  = VK_TRUE;
    GNT_ASSERT(m_GPUInfo.GPUFeatures.pipelineStatisticsQuery && m_GPUInfo.GPUFeatures.fillModeNonSolid &&
               m_GPUInfo.GPUFeatures.textureCompressionBC);

    deviceCI.pEnabledFeatures        = &PhysicalDeviceFeatures;
    deviceCI.enabledExtensionCount   = static_cast<uint32_t>(s_DeviceExtensions.size());
    deviceCI.ppEnabledExtensionNames = s_DeviceExtensions.data();

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
        case VK_PHYSICAL_DEVICE_TYPE_OTHER: score += 50; break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: score += 250; break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU: score += 400; break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: score += 800; break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: score += 1500; break;
    }

    // Maximum possible size of textures affects graphics quality
    score += gpuInfo.GPUProperties.limits.maxImageDimension2D;

    score += gpuInfo.GPUProperties.limits.maxViewports;
    score += gpuInfo.GPUProperties.limits.maxDescriptorSetSampledImages;

    // Maximum size of fast(-accessible) uniform data in shaders.
    score += gpuInfo.GPUProperties.limits.maxPushConstantsSize;

    // RTX part
    score += gpuInfo.RTProperties.maxRayRecursionDepth;

    return score;
}

bool VulkanDevice::IsDeviceSuitable(GPUInfo& gpuInfo, const VkSurfaceKHR& surface)
{
    // Query GPU properties(device name, limits, etc..)
    vkGetPhysicalDeviceProperties(gpuInfo.PhysicalDevice, &gpuInfo.GPUProperties);

    // Query GPU features(geometry shader support, multi-viewport support)
    vkGetPhysicalDeviceFeatures(gpuInfo.PhysicalDevice, &gpuInfo.GPUFeatures);

    // Query GPU memory properties
    vkGetPhysicalDeviceMemoryProperties(gpuInfo.PhysicalDevice, &gpuInfo.GPUMemoryProperties);

    VkPhysicalDeviceProperties2 GPUProperties2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    GPUProperties2.pNext                       = &gpuInfo.GPUDriverProperties;

    // Also query for RT pipeline properties
    gpuInfo.GPUDriverProperties.pNext = &gpuInfo.RTProperties;

    // Query info about acceleration structures
    gpuInfo.RTProperties.pNext = &gpuInfo.ASProperties;

    // Query info about mesh shading preferences
    gpuInfo.ASProperties.pNext = &gpuInfo.MSProperties;

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

    if (gpuInfo.RTProperties.maxRayRecursionDepth == 0)
        LOG_INFO(" [RTX]: Not supported :(");
    else
    {
        LOG_INFO(" [RTX]: Max Ray Bounce: %u", gpuInfo.RTProperties.maxRayRecursionDepth);
        LOG_INFO(" [RTX]: Shader Group Handle Size: %u", gpuInfo.RTProperties.shaderGroupHandleSize);
        LOG_INFO(" [RTX]: Max Primitive Count: %llu", gpuInfo.ASProperties.maxPrimitiveCount);
        LOG_INFO(" [RTX]: Max Geometry Count: %llu", gpuInfo.ASProperties.maxGeometryCount);
        LOG_INFO(" [RTX]: Max Instance(BLAS) Count: %llu", gpuInfo.ASProperties.maxInstanceCount);
    }

    // Mesh shader
    LOG_INFO(" [MS]: Max Output Vertices: %u", gpuInfo.MSProperties.maxMeshOutputVertices);
    LOG_INFO(" [MS]: Max Output Primitives: %u", gpuInfo.MSProperties.maxMeshOutputPrimitives);
    LOG_INFO(" [MS]: Max Output Memory Size: %u", gpuInfo.MSProperties.maxMeshOutputMemorySize);
    LOG_INFO(" [MS]: Max Mesh Work Group Count: (X, Y, Z) - (%u, %u, %u)", gpuInfo.MSProperties.maxMeshWorkGroupCount[0],
             gpuInfo.MSProperties.maxMeshWorkGroupCount[1], gpuInfo.MSProperties.maxMeshWorkGroupCount[2]);
    LOG_INFO(" [MS]: Max Mesh Work Group Invocations: %u", gpuInfo.MSProperties.maxMeshWorkGroupInvocations);
    LOG_INFO(" [MS]: Max Mesh Work Group Size: (X, Y, Z) - (%u, %u, %u)", gpuInfo.MSProperties.maxMeshWorkGroupSize[0],
             gpuInfo.MSProperties.maxMeshWorkGroupSize[1], gpuInfo.MSProperties.maxMeshWorkGroupSize[2]);
    LOG_INFO(" [MS]: Max Mesh Work Group Total Count: %u", gpuInfo.MSProperties.maxMeshWorkGroupTotalCount);
    LOG_WARN(" [MS]: Max Preferred Mesh Work Group Invocations: %u", gpuInfo.MSProperties.maxPreferredMeshWorkGroupInvocations);

    // Task shader
    LOG_INFO(" [TS]: Max Task Work Group Count: (X, Y, Z) - (%u, %u, %u)", gpuInfo.MSProperties.maxTaskWorkGroupCount[0],
             gpuInfo.MSProperties.maxTaskWorkGroupCount[1], gpuInfo.MSProperties.maxTaskWorkGroupCount[2]);
    LOG_INFO(" [TS]: Max Task Work Group Invocations: %u", gpuInfo.MSProperties.maxTaskWorkGroupInvocations);
    LOG_INFO(" [TS]: Max Task Work Group Size: (X, Y, Z) - (%u, %u, %u)", gpuInfo.MSProperties.maxTaskWorkGroupSize[0],
             gpuInfo.MSProperties.maxTaskWorkGroupSize[1], gpuInfo.MSProperties.maxTaskWorkGroupSize[2]);
    LOG_INFO(" [TS]: Max Task Work Group Total Count: %u", gpuInfo.MSProperties.maxTaskWorkGroupTotalCount);
    LOG_WARN(" [TS]: Max Preferred Task Work Group Invocations: %u", gpuInfo.MSProperties.maxPreferredTaskWorkGroupInvocations);
    LOG_INFO(" [TS]: Max Task Payload Size: %u", gpuInfo.MSProperties.maxTaskPayloadSize);

    LOG_WARN(" [MS]: Prefers Compact Primitive Output: %s", gpuInfo.MSProperties.prefersCompactPrimitiveOutput ? "TRUE" : "FALSE");
    LOG_WARN(" [MS]: Prefers Compact Vertex Output: %s", gpuInfo.MSProperties.prefersCompactVertexOutput ? "TRUE" : "FALSE");

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
        LOG_TRACE("  Size: %0.2f MB", gpuInfo.GPUMemoryProperties.memoryHeaps[i].size / 1024.0f / 1024.0f);

        if (gpuInfo.GPUMemoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) LOG_TRACE("    HEAP_DEVICE_LOCAL_BIT");

        if (gpuInfo.GPUMemoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT) LOG_TRACE("    HEAP_MULTI_INSTANCE_BIT");

        if ((gpuInfo.GPUMemoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) == 0 &&
            (gpuInfo.GPUMemoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT) == 0)
            LOG_TRACE("    HEAP_SYSTEM_RAM");
    }
#endif

    // Since all depth formats may be optional, we need to find a supported depth formats
    const std::vector<VkFormat> availableDepthFormats = {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT,
                                                         VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM};
    for (auto& format : availableDepthFormats)
    {
        VkFormatProperties formatProps = {};
        vkGetPhysicalDeviceFormatProperties(gpuInfo.PhysicalDevice, format, &formatProps);
        if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            gpuInfo.SupportedDepthFormats.emplace_back(format);
    }

    // No support for any depth format??
    if (gpuInfo.SupportedDepthFormats.empty()) return false;

    QueueFamilyIndices Indices = QueueFamilyIndices::FindQueueFamilyIndices(surface, gpuInfo.PhysicalDevice);
    gpuInfo.QueueFamilyIndices = Indices;

    // Check if we couldn't query queue family indices to create our command queues
    if (!Indices.IsComplete()) return false;

    // Check if device extensions are supported
    const bool bAreDeviceExtensionsSupported = CheckDeviceExtensionSupport(gpuInfo.PhysicalDevice);
    if (!bAreDeviceExtensionsSupported) return false;

    // Check if device can create swapchain
    SwapchainSupportDetails Details = SwapchainSupportDetails::QuerySwapchainSupportDetails(gpuInfo.PhysicalDevice, surface);
    if (Details.ImageFormats.empty() || Details.PresentModes.empty()) return false;

    if (VK_PREFER_IGPU && gpuInfo.GPUProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) return false;

    GNT_ASSERT(gpuInfo.GPUProperties.limits.maxSamplerAnisotropy > 0, "GPU has not valid Max Sampler Anisotropy!");
    return gpuInfo.GPUFeatures.samplerAnisotropy && gpuInfo.GPUFeatures.geometryShader;
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

#if LOG_VULKAN_INFO
    LOG_INFO("Available extensions on current gpu: (%u)", ExtensionCount);
#endif

    std::set<std::string> RequiredExtensions(s_DeviceExtensions.begin(), s_DeviceExtensions.end());
    for (const auto& Ext : AvailableExtensions)
    {
#if !LOG_VULKAN_INFO
        if (RequiredExtensions.empty()) return true;
#endif

#if LOG_VULKAN_INFO
        LOG_TRACE("%s", Ext.extensionName);
#endif

        RequiredExtensions.erase(Ext.extensionName);
    }

    return RequiredExtensions.empty();
}

void VulkanDevice::Destroy()
{
    WaitDeviceOnFinish();

    vkDestroyCommandPool(m_GPUInfo.LogicalDevice, m_GPUInfo.GraphicsCommandPool, nullptr);
    vkDestroyCommandPool(m_GPUInfo.LogicalDevice, m_GPUInfo.ComputeCommandPool, nullptr);
    vkDestroyCommandPool(m_GPUInfo.LogicalDevice, m_GPUInfo.TransferCommandPool, nullptr);

    vkDestroyDevice(m_GPUInfo.LogicalDevice, nullptr);
}

void VulkanDevice::AllocateCommandBuffer(VkCommandBuffer& inOutCommandBuffer, ECommandBufferType type, VkCommandBufferLevel level)
{
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    commandBufferAllocateInfo.level                       = level;
    commandBufferAllocateInfo.commandBufferCount          = 1;

    switch (type)
    {
        case ECommandBufferType::COMMAND_BUFFER_TYPE_GRAPHICS: commandBufferAllocateInfo.commandPool = m_GPUInfo.GraphicsCommandPool; break;
        case ECommandBufferType::COMMAND_BUFFER_TYPE_COMPUTE: commandBufferAllocateInfo.commandPool = m_GPUInfo.ComputeCommandPool; break;
        case ECommandBufferType::COMMAND_BUFFER_TYPE_TRANSFER: commandBufferAllocateInfo.commandPool = m_GPUInfo.TransferCommandPool; break;
        default: GNT_ASSERT(false, "Unknown command buffer type!");
    }

    VK_CHECK(vkAllocateCommandBuffers(m_GPUInfo.LogicalDevice, &commandBufferAllocateInfo, &inOutCommandBuffer),
             "Failed to allocate command buffer!");
}

void VulkanDevice::FreeCommandBuffer(const VkCommandBuffer& commandBuffer, ECommandBufferType type)
{
    switch (type)
    {
        case ECommandBufferType::COMMAND_BUFFER_TYPE_GRAPHICS:
        {
            vkFreeCommandBuffers(m_GPUInfo.LogicalDevice, m_GPUInfo.GraphicsCommandPool, 1, &commandBuffer);
            break;
        }
        case ECommandBufferType::COMMAND_BUFFER_TYPE_COMPUTE:
        {
            vkFreeCommandBuffers(m_GPUInfo.LogicalDevice, m_GPUInfo.ComputeCommandPool, 1, &commandBuffer);
            break;
        }
        case ECommandBufferType::COMMAND_BUFFER_TYPE_TRANSFER:
        {
            vkFreeCommandBuffers(m_GPUInfo.LogicalDevice, m_GPUInfo.TransferCommandPool, 1, &commandBuffer);
            break;
        }
    }
}

}  // namespace Gauntlet
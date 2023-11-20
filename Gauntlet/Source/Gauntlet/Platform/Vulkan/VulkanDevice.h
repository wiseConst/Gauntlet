#pragma once

#include <Gauntlet/Core/Core.h>
#include <volk/volk.h>
#include "VulkanUtility.h"

namespace Gauntlet
{
struct QueueFamilyIndices
{
    FORCEINLINE bool IsComplete() const
    {
        return GraphicsFamily != UINT32_MAX && PresentFamily != UINT32_MAX && ComputeFamily != UINT32_MAX && TransferFamily != UINT32_MAX;
    }

    static QueueFamilyIndices FindQueueFamilyIndices(const VkSurfaceKHR& surface, const VkPhysicalDevice& physicalDevice)
    {
        QueueFamilyIndices Indices = {};

        uint32_t QueueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &QueueFamilyCount, nullptr);
        GNT_ASSERT(QueueFamilyCount != 0, "Looks like your gpu doesn't have any queues to submit commands to.");

        std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &QueueFamilyCount, QueueFamilies.data());

#if LOG_VULKAN_INFO
        LOG_INFO("QueueFamilyCount:%u", QueueFamilyCount);
#endif

        uint32_t i = 0;
        for (const auto& QueueFamily : QueueFamilies)
        {
#if LOG_VULKAN_INFO
            LOG_TRACE(" [%d] queueFamily has (%u) queue(s) which supports operations:", i + 1, QueueFamily.queueCount);
#endif

            if (QueueFamily.queueCount <= 0)
            {
                ++i;
                continue;
            }

            if (QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                Indices.GraphicsFamily = i;
#if LOG_VULKAN_INFO
                LOG_TRACE("  GRAPHICS");
#endif
            }

            if (QueueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                Indices.ComputeFamily = i;
#if LOG_VULKAN_INFO
                LOG_TRACE("  COMPUTE");
#endif
            }

            if (QueueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                // Dedicated transfer queue.
                if (!(QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(QueueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT))
                    Indices.TransferFamily = i;

#if LOG_VULKAN_INFO
                LOG_TRACE("  TRANSFER");
#endif
            }

            VkBool32 bPresentSupport{VK_FALSE};
            {
                const auto result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &bPresentSupport);
                GNT_ASSERT(result == VK_SUCCESS, "Failed to check if current GPU supports image presenting.");
            }

            if (bPresentSupport)
            {
                Indices.PresentFamily = i;

#if LOG_VULKAN_INFO
                LOG_TRACE("  PRESENT");
#endif
            }

#if LOG_VULKAN_INFO
            if (QueueFamily.queueFlags & VK_QUEUE_PROTECTED_BIT)
            {
                LOG_TRACE("  PROTECTED");
            }

            if (QueueFamily.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
            {
                LOG_TRACE("  SPARSE BINDING");
            }

            if (QueueFamily.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR)
            {
                LOG_TRACE("  VIDEO DECODE");
            }
#ifdef VK_ENABLE_BETA_EXTENSIONS
            if (QueueFamily.queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR)
            {
                LOG_TRACE("  VIDEO ENCODE");
            }
#endif
            if (QueueFamily.queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV)
            {
                LOG_TRACE("  OPTICAL FLOW NV");
            }
#endif

#if !LOG_VULKAN_INFO
            if (Indices.IsComplete()) break;
#endif

            ++i;
        }

        // From vulkan-tutorial.com: Any queue family with VK_QUEUE_GRAPHICS_BIT or VK_QUEUE_COMPUTE_BIT capabilities already implicitly
        // support VK_QUEUE_TRANSFER_BIT operations.
        if (Indices.TransferFamily == UINT32_MAX) Indices.TransferFamily = Indices.GraphicsFamily;

        // Rely on that graphics queue will be from first family, which contains everything.
        if (Indices.ComputeFamily == UINT32_MAX) Indices.ComputeFamily = Indices.GraphicsFamily;

        // iGPU issues
        if (Indices.PresentFamily != Indices.GraphicsFamily) Indices.PresentFamily = Indices.GraphicsFamily;

        GNT_ASSERT(Indices.IsComplete(), "QueueFamilyIndices wasn't setup correctly!");
        return Indices;
    }

    uint32_t GraphicsFamily = UINT32_MAX;
    uint32_t PresentFamily  = UINT32_MAX;
    uint32_t ComputeFamily  = UINT32_MAX;
    uint32_t TransferFamily = UINT32_MAX;
};

class VulkanDevice final : private Uncopyable, private Unmovable
{
  public:
    VulkanDevice(const VkInstance& instance, const VkSurfaceKHR& surface);
    VulkanDevice() = delete;

    ~VulkanDevice() = default;

    void Destroy();

    FORCEINLINE void WaitDeviceOnFinish() const
    {
        GNT_ASSERT(IsValid(), "Rendering device is not valid!");

        const auto result = vkDeviceWaitIdle(m_GPUInfo.LogicalDevice);
        GNT_ASSERT(result == VK_SUCCESS, "Failed to wait for rendering device to finish other operations.");
    }

    FORCEINLINE bool IsValid() const { return m_GPUInfo.PhysicalDevice && m_GPUInfo.LogicalDevice; }

    FORCEINLINE const auto& GetPhysicalDevice() const { return m_GPUInfo.PhysicalDevice; }
    FORCEINLINE auto& GetPhysicalDevice() { return m_GPUInfo.PhysicalDevice; }

    FORCEINLINE const auto& GetLogicalDevice() const { return m_GPUInfo.LogicalDevice; }
    FORCEINLINE auto& GetLogicalDevice() { return m_GPUInfo.LogicalDevice; }

    FORCEINLINE auto& GetQueueFamilyIndices() { return m_GPUInfo.QueueFamilyIndices; }
    FORCEINLINE const auto& GetQueueFamilyIndices() const { return m_GPUInfo.QueueFamilyIndices; }

    FORCEINLINE const auto& GetGraphicsQueue() const { return m_GPUInfo.GraphicsQueue; }
    FORCEINLINE auto& GetGraphicsQueue() { return m_GPUInfo.GraphicsQueue; }

    FORCEINLINE const auto& GetPresentQueue() const { return m_GPUInfo.PresentQueue; }
    FORCEINLINE auto& GetPresentQueue() { return m_GPUInfo.PresentQueue; }

    FORCEINLINE const auto& GetTransferQueue() const { return m_GPUInfo.TransferQueue; }
    FORCEINLINE auto& GetTransferQueue() { return m_GPUInfo.TransferQueue; }

    FORCEINLINE const auto& GetComputeQueue() const { return m_GPUInfo.ComputeQueue; }
    FORCEINLINE auto& GetComputeQueue() { return m_GPUInfo.ComputeQueue; }

    FORCEINLINE const auto& GetMemoryProperties() const { return m_GPUInfo.GPUMemoryProperties; }
    FORCEINLINE auto& GetMemoryProperties() { return m_GPUInfo.GPUMemoryProperties; }

    FORCEINLINE const auto& GetGPUProperties() const { return m_GPUInfo.GPUProperties; }
    FORCEINLINE auto& GetGPUProperties() { return m_GPUInfo.GPUProperties; }

    FORCEINLINE const auto& GetGPUFeatures() const { return m_GPUInfo.GPUFeatures; }
    FORCEINLINE auto& GetGPUFeatures() { return m_GPUInfo.GPUFeatures; }

    FORCEINLINE VkDeviceAddress GetBufferDeviceAddress(const VkBuffer& buffer)
    {
        VkBufferDeviceAddressInfo bdaInfo = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
        bdaInfo.buffer                    = buffer;
        return vkGetBufferDeviceAddress(m_GPUInfo.LogicalDevice, &bdaInfo);
    }

  private:
    struct GPUInfo
    {
        VkDevice LogicalDevice          = VK_NULL_HANDLE;
        VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;

        QueueFamilyIndices QueueFamilyIndices = {};
        VkQueue GraphicsQueue                 = VK_NULL_HANDLE;
        VkQueue PresentQueue                  = VK_NULL_HANDLE;
        VkQueue TransferQueue                 = VK_NULL_HANDLE;
        VkQueue ComputeQueue                  = VK_NULL_HANDLE;

        VkPhysicalDeviceProperties GPUProperties                     = {};
        VkPhysicalDeviceFeatures GPUFeatures                         = {};
        VkPhysicalDeviceMemoryProperties GPUMemoryProperties         = {};
        VkPhysicalDeviceDriverProperties GPUDriverProperties         = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES};
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR RTProperties = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
        VkPhysicalDeviceAccelerationStructurePropertiesKHR ASProperties = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
    } m_GPUInfo;

    void PickPhysicalDevice(const VkInstance& instance, const VkSurfaceKHR& surface);
    void CreateLogicalDevice();

    uint32_t RateDeviceSuitability(GPUInfo& gpuInfo, const VkSurfaceKHR& surface);
    bool CheckDeviceExtensionSupport(const VkPhysicalDevice& physicalDevice);
    bool IsDeviceSuitable(GPUInfo& gpuInfo, const VkSurfaceKHR& surface);
};

}  // namespace Gauntlet

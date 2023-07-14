#pragma once

#include <Eclipse/Core/Core.h>
#include <volk/volk.h>

namespace Eclipse
{
struct QueueFamilyIndices
{
  public:
    QueueFamilyIndices() : GraphicsFamily(UINT32_MAX), PresentFamily(UINT32_MAX), ComputeFamily(UINT32_MAX), TransferFamily(UINT32_MAX) {}

    FORCEINLINE bool IsComplete() const
    {
        return GraphicsFamily != UINT32_MAX && PresentFamily != UINT32_MAX && ComputeFamily != UINT32_MAX && TransferFamily != UINT32_MAX;
    }

    static QueueFamilyIndices FindQueueFamilyIndices(const VkSurfaceKHR& InSurface, const VkPhysicalDevice& InPhysicalDevice)
    {
        QueueFamilyIndices Indices = {};

        uint32_t QueueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(InPhysicalDevice, &QueueFamilyCount, nullptr);
        ELS_ASSERT(QueueFamilyCount != 0, "Looks like your gpu doesn't have any queues to submit commands to.");

        std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(InPhysicalDevice, &QueueFamilyCount, QueueFamilies.data());

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

            // Dedicated transfer queue.
            if (QueueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT && !(QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT))
            {
                Indices.TransferFamily = i;
#if LOG_VULKAN_INFO
                LOG_TRACE("  TRANSFER");
#endif
            }

            VkBool32 bPresentSupport{VK_FALSE};
            {
                const auto result = vkGetPhysicalDeviceSurfaceSupportKHR(InPhysicalDevice, i, InSurface, &bPresentSupport);
                ELS_ASSERT(result == VK_SUCCESS, "Failed to check if current GPU supports image presenting.");
            }

            if (bPresentSupport)
            {
                Indices.PresentFamily = i;
#if LOG_VULKAN_INFO
                LOG_TRACE("  PRESENT");
#endif
            }

#if !LOG_VULKAN_INFO
            if (Indices.IsComplete()) break;
#endif

            ++i;
        }

        // From vulkan-tutorial.com: Any queue family with VK_QUEUE_GRAPHICS_BIT or VK_QUEUE_COMPUTE_BIT capabilities already implicitly
        // support VK_QUEUE_TRANSFER_BIT operations.
        if (Indices.TransferFamily == UINT32_MAX) Indices.TransferFamily = Indices.GraphicsFamily;

        ELS_ASSERT(Indices.IsComplete(), "QueueFamilyIndices wasn't setup correctly!");
        return Indices;
    }

    uint32_t GraphicsFamily;
    uint32_t PresentFamily;
    uint32_t ComputeFamily;
    uint32_t TransferFamily;
};

class VulkanDevice final : private Uncopyable, private Unmovable
{
  public:
    VulkanDevice(const VkInstance& InInstance, const VkSurfaceKHR& InSurface);
    VulkanDevice() = delete;

    ~VulkanDevice() = default;

    void Destroy();

    FORCEINLINE void WaitDeviceOnFinish() const
    {
        const auto result = vkDeviceWaitIdle(m_GPUInfo.LogicalDevice);
        ELS_ASSERT(result == VK_SUCCESS, "Failed to wait for device to finish other operations.");
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

    FORCEINLINE const auto& GetMemoryProperties() const { return m_GPUInfo.GPUMemoryProperties; }
    FORCEINLINE auto& GetMemoryProperties() { return m_GPUInfo.GPUMemoryProperties; }

    FORCEINLINE const auto& GetGPUProperties() const { return m_GPUInfo.GPUProperties; }
    FORCEINLINE auto& GetGPUProperties() { return m_GPUInfo.GPUProperties; }

    FORCEINLINE const size_t GetUniformBufferAlignedSize(const size_t InOriginalSize)
    {
        // Calculate required alignment based on minimum device offset alignment
        size_t alignedSize = InOriginalSize;
        if (m_GPUInfo.GPUProperties.limits.minUniformBufferOffsetAlignment > 0)
        {
            alignedSize = (alignedSize + m_GPUInfo.GPUProperties.limits.minUniformBufferOffsetAlignment - 1) &
                          ~(m_GPUInfo.GPUProperties.limits.minUniformBufferOffsetAlignment - 1);
        }
        return alignedSize;
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

        VkPhysicalDeviceProperties GPUProperties             = {};
        VkPhysicalDeviceFeatures GPUFeatures                 = {};
        VkPhysicalDeviceMemoryProperties GPUMemoryProperties = {};
        VkPhysicalDeviceDriverProperties GPUDriverProperties = {};
    } m_GPUInfo;

    void PickPhysicalDevice(const VkInstance& InInstance, const VkSurfaceKHR& InSurface);
    void CreateLogicalDevice(const VkSurfaceKHR& InSurface);

    uint32_t RateDeviceSuitability(GPUInfo& InGPUInfo, const VkSurfaceKHR& InSurface);
    bool CheckDeviceExtensionSupport(const VkPhysicalDevice& InPhysicalDevice);
    bool IsDeviceSuitable(GPUInfo& InGPUInfo, const VkSurfaceKHR& InSurface);
};

}  // namespace Eclipse

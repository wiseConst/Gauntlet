#pragma once

#include <Eclipse/Core/Core.h>
#include <volk/volk.h>

#include "QueueFamilyIndices.h"

namespace Eclipse
{
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
        VkDevice LogicalDevice = VK_NULL_HANDLE;
        VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;

        QueueFamilyIndices QueueFamilyIndices = {};
        VkQueue GraphicsQueue = VK_NULL_HANDLE;
        VkQueue PresentQueue = VK_NULL_HANDLE;
        VkQueue TransferQueue = VK_NULL_HANDLE;

        VkPhysicalDeviceProperties GPUProperties = {};
        VkPhysicalDeviceFeatures GPUFeatures = {};
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

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
        const auto result = vkDeviceWaitIdle(m_LogicalDevice);
        ELS_ASSERT(result == VK_SUCCESS, "Failed to wait for device to finish other operations.");
    }

    FORCEINLINE bool IsValid() const { return m_PhysicalDevice && m_LogicalDevice; }

    FORCEINLINE const auto& GetPhysicalDevice() const { return m_PhysicalDevice; }
    FORCEINLINE auto& GetPhysicalDevice() { return m_PhysicalDevice; }

    FORCEINLINE const auto& GetLogicalDevice() const { return m_LogicalDevice; }
    FORCEINLINE auto& GetLogicalDevice() { return m_LogicalDevice; }

    FORCEINLINE auto& GetQueueFamilyIndices() { return m_QueueFamilyIndices; }
    FORCEINLINE const auto& GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }

    FORCEINLINE const auto& GetGraphicsQueue() const { return m_GraphicsQueue; }
    FORCEINLINE auto& GetGraphicsQueue() { return m_GraphicsQueue; }

    FORCEINLINE const auto& GetPresentQueue() const { return m_PresentQueue; }
    FORCEINLINE auto& GetPresentQueue() { return m_PresentQueue; }

    FORCEINLINE const auto& GetTransferQueue() const { return m_TransferQueue; }
    FORCEINLINE auto& GetTransferQueue() { return m_TransferQueue; }

    FORCEINLINE const auto& GetMemoryProperties() const { return m_GPUMemoryProperties; }
    FORCEINLINE auto& GetMemoryProperties() { return m_GPUMemoryProperties; }

    FORCEINLINE const auto& GetGPUProperties() const { return m_GPUProperties; }
    FORCEINLINE auto& GetGPUProperties() { return m_GPUProperties; }

  private:
    VkDevice m_LogicalDevice = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;

    QueueFamilyIndices m_QueueFamilyIndices;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    VkQueue m_PresentQueue = VK_NULL_HANDLE;
    VkQueue m_TransferQueue = VK_NULL_HANDLE;

    VkPhysicalDeviceProperties m_GPUProperties;
    VkPhysicalDeviceFeatures m_GPUFeatures;
    VkPhysicalDeviceMemoryProperties m_GPUMemoryProperties;

    void PickPhysicalDevice(const VkInstance& InInstance, const VkSurfaceKHR& InSurface);
    void CreateLogicalDevice(const VkSurfaceKHR& InSurface);

    bool IsDeviceSuitable(const VkPhysicalDevice& InPhysicalDevice, const VkSurfaceKHR& InSurface);
    bool CheckDeviceExtensionSupport(const VkPhysicalDevice& InPhysicalDevice);
    uint32_t RateDeviceSuitability(const VkPhysicalDevice& InPhysicalDevice, const VkSurfaceKHR& InSurface);
};

}  // namespace Eclipse

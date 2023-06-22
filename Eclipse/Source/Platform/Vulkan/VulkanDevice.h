#pragma once

#include <Eclipse/Core/Core.h>
#include <volk/volk.h>

#include "QueueFamilyIndices.h"

namespace Eclipse
{
class VulkanDevice final
{
  public:
    VulkanDevice(const VkInstance& InInstance, const VkSurfaceKHR& InSurface);
    VulkanDevice() = delete;

    VulkanDevice(const VulkanDevice& that) = delete;
    VulkanDevice(VulkanDevice&& that) = delete;

    VulkanDevice& operator=(const VulkanDevice& that) = delete;
    VulkanDevice& operator=(VulkanDevice&& that) = delete;

    ~VulkanDevice() = default;

    void Destroy();

    FORCEINLINE void WaitDeviceOnFinish() const
    {
        const auto result = vkDeviceWaitIdle(m_LogicalDevice);
        ELS_ASSERT(result == VK_SUCCESS, "Failed to wait for device to finish other operations.");
    }

    FORCEINLINE VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
    FORCEINLINE VkDevice GetLogicalDevice() const { return m_LogicalDevice; }
    FORCEINLINE QueueFamilyIndices GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }

    FORCEINLINE const auto& GetGraphicsQueue() const { return m_GraphicsQueue; }
    FORCEINLINE auto& GetGraphicsQueue() { return m_GraphicsQueue; }

    FORCEINLINE const auto& GetPresentQueue() const { return m_PresentQueue; }
    FORCEINLINE auto& GetPresentQueue() { return m_PresentQueue; }

    FORCEINLINE const auto& GetTransferQueue() const { return m_TransferQueue; }
    FORCEINLINE auto& GetTransferQueue() { return m_TransferQueue; }

    FORCEINLINE const auto& GetMemoryProperties() const { return m_GPUMemoryProperties; }
    FORCEINLINE const auto& GetGPUProperties() const { return m_GPUProperties; }

  private:
    VkDevice m_LogicalDevice = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;

    VkPhysicalDeviceProperties m_GPUProperties;
    VkPhysicalDeviceFeatures m_GPUFeatures;
    VkPhysicalDeviceMemoryProperties m_GPUMemoryProperties;

    QueueFamilyIndices m_QueueFamilyIndices;
    
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    VkQueue m_PresentQueue = VK_NULL_HANDLE;
    VkQueue m_TransferQueue = VK_NULL_HANDLE;

    void PickPhysicalDevice(const VkInstance& InInstance, const VkSurfaceKHR& InSurface);
    void CreateLogicalDevice(const VkSurfaceKHR& InSurface);

    bool IsDeviceSuitable(const VkPhysicalDevice& InPhysicalDevice, const VkSurfaceKHR& InSurface);
    bool CheckDeviceExtensionSupport(const VkPhysicalDevice& InPhysicalDevice);
    const char* GetVendorNameCString(uint32_t vendorID);
    const char* GetDeviceTypeCString(VkPhysicalDeviceType deviceType);
    uint32_t RateDeviceSuitability(const VkPhysicalDevice& InPhysicalDevice, const VkSurfaceKHR& InSurface);
};

}  // namespace Eclipse

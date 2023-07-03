#pragma once

#include "Eclipse/Core/Core.h"
#include <volk/volk.h>

#include "VulkanUtility.h"

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

    FORCEINLINE void SetGraphicsFamily(uint32_t index) { GraphicsFamily = index; }
    FORCEINLINE uint32_t GetGraphicsFamily() const { return GraphicsFamily; }

    FORCEINLINE void SetPresentFamily(uint32_t index) { PresentFamily = index; }
    FORCEINLINE uint32_t GetPresentFamily() const { return PresentFamily; }

    FORCEINLINE void SetComputeFamily(uint32_t index) { ComputeFamily = index; }
    FORCEINLINE uint32_t GetComputeFamily() const { return ComputeFamily; }

    FORCEINLINE void SetTransferFamily(uint32_t index) { TransferFamily = index; }
    FORCEINLINE uint32_t GetTransferFamily() const { return TransferFamily; }

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

        // TODO: Sort out about queue families

        int32_t i = 0;
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
                Indices.SetGraphicsFamily(i);
#if LOG_VULKAN_INFO
                LOG_TRACE("  GRAPHICS");
#endif
            }

            if (QueueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                Indices.SetComputeFamily(i);
#if LOG_VULKAN_INFO
                LOG_TRACE("  COMPUTE");
#endif
            }

            if (QueueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                Indices.SetTransferFamily(i);
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
                Indices.SetPresentFamily(i);
#if LOG_VULKAN_INFO
                LOG_TRACE("  PRESENT");
#endif
            }

            if (Indices.IsComplete()) break;

            ++i;
        }

        return Indices;
    }

  private:
    uint32_t GraphicsFamily;
    uint32_t PresentFamily;
    uint32_t ComputeFamily;
    uint32_t TransferFamily;
};
}  // namespace Eclipse
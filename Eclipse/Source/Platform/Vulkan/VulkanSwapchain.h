#pragma once

#include "Eclipse/Core/Core.h"

#include <volk/volk.h>

#include "SwapchainSupportDetails.h"
#include "VulkanTexture.h"

// TODO: Add struct SwapchainSpecification to make it configurable

/* struct SwapchainSpecification
{
  public:
    enum EImageUsage // or the only SwapchainImageUsage ??
    {
        IMAGE_USAGE_TRANSFER_SRC = 0,
        IMAGE_USAGE_TRANSFER_DST,
        IMAGE_USAGE_SAMPLED,

    };

    bool clipped ?

    int32_t imageArrayLayers?
};*/

namespace Eclipse
{
class VulkanDevice;

class VulkanSwapchain final : private Uncopyable, private Unmovable
{
  public:
    VulkanSwapchain(Scoped<VulkanDevice>& InDevice, VkSurfaceKHR& InSurface,
                    const ImageSpecification& InImageSpecification = ImageSpecification());
    ~VulkanSwapchain() = default;

    FORCEINLINE bool IsValid() const { return m_Swapchain && m_SwapchainImages.size() > 0; }
    FORCEINLINE const auto& GetImageFormat() const { return m_SwapchainImageFormat.format; }
    FORCEINLINE auto& GetImageFormat() { return m_SwapchainImageFormat.format; }

    FORCEINLINE const auto& GetImageExtent() const { return m_SwapchainImageExtent; }
    FORCEINLINE auto& GetImageExtent() { return m_SwapchainImageExtent; }

    FORCEINLINE const auto& GetImageViews() const { return m_SwapchainImageViews; }
    FORCEINLINE auto& GetImageViews() { return m_SwapchainImageViews; }

    FORCEINLINE auto GetCurrentImageIndex() const { return m_ImageIndex; }
    FORCEINLINE auto GetCurrentFrameIndex() const { return m_FrameIndex; }
    FORCEINLINE auto GetImageCount() const { return m_SwapchainImageCount; }

    FORCEINLINE const auto& GetDepthFormat() const { return m_DepthImage->GetFormat(); }
    FORCEINLINE auto& GetDepthFormat() { return m_DepthImage->GetFormat(); }

    FORCEINLINE const auto& GetDepthImageView() const { return m_DepthImage->GetView(); }
    FORCEINLINE auto& GetDepthImageView() { return m_DepthImage->GetView(); }

    bool TryAcquireNextImage(const VkSemaphore& InImageAcquiredSemaphore, const VkFence& InFence = VK_NULL_HANDLE);
    bool TryPresentImage(const VkSemaphore& InRenderFinishedSemaphore);

    void Create();
    void Destroy();

  private:
    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    VkSwapchainKHR m_OldSwapchain = VK_NULL_HANDLE;

    Scoped<VulkanImage> m_DepthImage;
    Scoped<VulkanDevice>& m_Device;
    VkSurfaceKHR& m_Surface;

    uint32_t m_SwapchainImageCount{0};
    VkPresentModeKHR m_CurrentPresentMode;
    VkSurfaceFormatKHR m_SwapchainImageFormat;
    VkExtent2D m_SwapchainImageExtent;

    std::vector<VkImage> m_SwapchainImages;
    std::vector<VkImageView> m_SwapchainImageViews;

    uint32_t m_ImageIndex{0};
    uint32_t m_FrameIndex{0};
};
}  // namespace Eclipse
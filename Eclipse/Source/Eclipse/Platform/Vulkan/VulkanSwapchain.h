#pragma once

#include "Eclipse/Core/Core.h"

#include <volk/volk.h>

#include "SwapchainSupportDetails.h"
#include "VulkanImage.h"
#include "VulkanFramebuffer.h"

namespace Eclipse
{
class VulkanDevice;

class VulkanSwapchain final : private Uncopyable, private Unmovable
{
  public:
    VulkanSwapchain(Scoped<VulkanDevice>& InDevice, VkSurfaceKHR& InSurface);
    ~VulkanSwapchain() = default;

    FORCEINLINE bool IsValid() const { return m_Swapchain && m_SwapchainImages.size() > 0; }
    FORCEINLINE const auto& GetImageFormat() const { return m_SwapchainImageFormat.format; }
    FORCEINLINE auto& GetImageFormat() { return m_SwapchainImageFormat.format; }
    FORCEINLINE const float GetAspectRatio() const
    {
        return m_SwapchainImageExtent.height == 0 ? 1.0f : m_SwapchainImageExtent.width / (float)m_SwapchainImageExtent.height;
    }

    FORCEINLINE const auto& GetImageExtent() const { return m_SwapchainImageExtent; }
    FORCEINLINE auto& GetImageExtent() { return m_SwapchainImageExtent; }

    FORCEINLINE const auto& GetImageViews() const { return m_SwapchainImageViews; }
    FORCEINLINE auto& GetImageViews() { return m_SwapchainImageViews; }

    FORCEINLINE auto GetCurrentImageIndex() const { return m_ImageIndex; }
    FORCEINLINE auto GetCurrentFrameIndex() const { return m_FrameIndex; }
    FORCEINLINE auto GetImageCount() const { return m_SwapchainImageCount; }

    FORCEINLINE const auto GetDepthFormat() const { return m_DepthImage->GetFormat(); }
    FORCEINLINE auto GetDepthFormat() { return m_DepthImage->GetFormat(); }

    FORCEINLINE const auto& GetDepthImageView() const { return m_DepthImage->GetView(); }
    FORCEINLINE auto& GetDepthImageView() { return m_DepthImage->GetView(); }

    FORCEINLINE const auto& GetRenderPass() const { return m_RenderPass; }
    FORCEINLINE auto& GetRenderPass() { return m_RenderPass; }

    FORCEINLINE void SetClearColor(const glm::vec4& InClearColor)
    {
        m_ClearColor = {InClearColor.r, InClearColor.g, InClearColor.b, InClearColor.a};
    }

    void BeginRenderPass(const VkCommandBuffer& InCommandBuffer);
    FORCEINLINE void EndRenderPass(const VkCommandBuffer& InCommandBuffer) { vkCmdEndRenderPass(InCommandBuffer); }

    bool TryAcquireNextImage(const VkSemaphore& InImageAcquiredSemaphore, const VkFence& InFence = VK_NULL_HANDLE);
    bool TryPresentImage(const VkSemaphore& InRenderFinishedSemaphore);

    void Invalidate();
    void Destroy();

  private:
    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;

    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_Framebuffers;
    VkClearColorValue m_ClearColor;

    Scoped<VulkanDevice>& m_Device;
    VkSurfaceKHR& m_Surface;

    uint32_t m_SwapchainImageCount{0};
    VkPresentModeKHR m_CurrentPresentMode;
    VkSurfaceFormatKHR m_SwapchainImageFormat;
    VkExtent2D m_SwapchainImageExtent;

    Scoped<VulkanImage> m_DepthImage;
    std::vector<VkImage> m_SwapchainImages;
    std::vector<VkImageView> m_SwapchainImageViews;

    uint32_t m_ImageIndex{0};
    uint32_t m_FrameIndex{0};

    void InvalidateRenderPass();
    void DestroyRenderPass();
};
}  // namespace Eclipse
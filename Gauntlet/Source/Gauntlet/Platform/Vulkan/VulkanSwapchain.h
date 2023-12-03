#pragma once

#include "Gauntlet/Core/Core.h"

#include <volk/volk.h>

#include "VulkanImage.h"
#include "VulkanFramebuffer.h"

namespace Gauntlet
{
class VulkanDevice;
class VulkanCommandBuffer;

class VulkanSwapchain final : private Uncopyable, private Unmovable
{
  private:
    using ResizeCallback = std::function<void()>;

  public:
    VulkanSwapchain(Scoped<VulkanDevice>& device, VkSurfaceKHR& surface);
    ~VulkanSwapchain() = default;

    FORCEINLINE bool IsValid() const { return m_Swapchain && m_SwapchainImages.size() > 0; }
    FORCEINLINE const auto& GetImageFormat() const { return m_SwapchainImageFormat.format; }

    FORCEINLINE const float GetAspectRatio() const
    {
        return m_SwapchainImageExtent.height == 0 ? 1.0f : m_SwapchainImageExtent.width / (float)m_SwapchainImageExtent.height;
    }

    FORCEINLINE const auto& GetImageExtent() const { return m_SwapchainImageExtent; }
    FORCEINLINE const auto& GetImageViews() const { return m_SwapchainImageViews; }

    FORCEINLINE auto GetCurrentImageIndex() const { return m_ImageIndex; }
    FORCEINLINE auto GetCurrentFrameIndex() const { return m_FrameIndex; }
    FORCEINLINE auto GetImageCount() const { return m_SwapchainImageCount; }

    FORCEINLINE const auto& GetRenderPass() { return m_RenderPass; }

    FORCEINLINE void SetClearColor(const glm::vec4& clearColor) { m_ClearColor = {clearColor.r, clearColor.g, clearColor.b, clearColor.a}; }

    void BeginRenderPass(const VkCommandBuffer& commandBuffer);
    FORCEINLINE void EndRenderPass(const VkCommandBuffer& commandBuffer) { vkCmdEndRenderPass(commandBuffer); }

    bool TryAcquireNextImage(const VkSemaphore& imageAcquiredSemaphore, const VkFence& fence = VK_NULL_HANDLE);
    void PresentImage(const VkSemaphore& renderFinishedSemaphore);

    void Invalidate();
    void Destroy();

    FORCEINLINE void AddResizeCallback(const std::function<void()>& resizeCallback) { m_ResizeCallbacks.push_back(resizeCallback); }
    FORCEINLINE const auto& GetCommandBuffers() const { return m_CommandBuffers; }

  private:
    std::vector<ResizeCallback> m_ResizeCallbacks;
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

    std::vector<VkImage> m_SwapchainImages;
    std::vector<VkImageView> m_SwapchainImageViews;

    uint32_t m_ImageIndex{0};
    uint32_t m_FrameIndex{0};

    std::vector<Ref<VulkanCommandBuffer>> m_CommandBuffers;

    void InvalidateRenderPass();
    void DestroyRenderPass();

    void Recreate();
};
}  // namespace Gauntlet
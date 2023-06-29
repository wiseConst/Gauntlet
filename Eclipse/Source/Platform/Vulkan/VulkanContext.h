#pragma once

#include "Eclipse/Core/Core.h"
#include "Eclipse/Renderer/GraphicsContext.h"

#ifdef ELS_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <volk/volk.h>

namespace Eclipse
{

class VulkanDevice;
class VulkanAllocator;
class VulkanSwapchain;
class VulkanCommandPool;
class VulkanRenderPass;

class VulkanContext final : public GraphicsContext
{
  public:
    VulkanContext(Scoped<Window>& InWindow);

    void BeginRender() final override;
    void EndRender() final override;

    void SwapBuffers() final override;
    void SetVSync(bool IsVSync) final override;
    void Destroy() final override;

    FORCEINLINE const auto& GetDevice() const { return m_Device; }
    FORCEINLINE auto& GetDevice() { return m_Device; }

    FORCEINLINE const auto& GetSwapchain() const { return m_Swapchain; }
    FORCEINLINE auto& GetSwapchain() { return m_Swapchain; }

    FORCEINLINE const auto& GetAllocator() const { return m_Allocator; }
    FORCEINLINE auto& GetAllocator() { return m_Allocator; }

    FORCEINLINE const auto& GetTransferCommandPool() const { return m_TransferCommandPool; }
    FORCEINLINE auto& GetTransferCommandPool() { return m_TransferCommandPool; }

    FORCEINLINE const auto& GetGraphicsCommandPool() const { return m_GraphicsCommandPool; }
    FORCEINLINE auto& GetGraphicsCommandPool() { return m_GraphicsCommandPool; }

  private:
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

    Scoped<VulkanDevice> m_Device;
    Scoped<VulkanAllocator> m_Allocator;
    Scoped<VulkanSwapchain> m_Swapchain;
    Scoped<VulkanCommandPool> m_TransferCommandPool;
    Scoped<VulkanCommandPool> m_GraphicsCommandPool;

    Scoped<VulkanRenderPass> m_GlobalRenderPass;

    // Sync objects GPU-GPU.
    VkSemaphore m_RenderFinishedSemaphore;
    VkSemaphore m_ImageAcquiredSemaphore;

    // Sync objects CPU-GPU
    VkFence m_InFlightFence;

    void CreateInstance();
    void CreateDebugMessenger();
    void CreateSurface();
    void CreateSyncObjects();
    void CreateGlobalRenderPass();

    void RecreateSwapchain();

    bool CheckVulkanAPISupport() const;
    bool CheckValidationLayerSupport();
    const std::vector<const char*> GetRequiredExtensions();
};

}  // namespace Eclipse
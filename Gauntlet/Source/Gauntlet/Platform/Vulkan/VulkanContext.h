#pragma once

#include "Gauntlet/Core/Core.h"
#include "Gauntlet/Renderer/GraphicsContext.h"

#include <volk/volk.h>
#include <glm/glm.hpp>

#include "VulkanCommandBuffer.h"

namespace Gauntlet
{

class VulkanDevice;
class VulkanAllocator;
class VulkanSwapchain;
class VulkanCommandPool;
class VulkanDescriptorAllocator;

class VulkanContext final : public GraphicsContext
{
  private:
    using ResizeCallback = std::function<void()>;

  public:
    VulkanContext() = delete;
    VulkanContext(Scoped<Window>& window);
    ~VulkanContext() = default;

    void BeginRender() final override;
    void EndRender() final override;

    void SwapBuffers() final override;
    void SetVSync(bool bIsVSync) final override;
    void Destroy() final override;

    FORCEINLINE const auto& GetInstance() const { return m_Instance; }
    FORCEINLINE auto& GetInstance() { return m_Instance; }

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

    FORCEINLINE const auto& GetDescriptorAllocator() const { return m_DescriptorAllocator; }
    FORCEINLINE auto& GetDescriptorAllocator() { return m_DescriptorAllocator; }

    FORCEINLINE const auto& GetThreadData() const { return m_ThreadData; }
    FORCEINLINE auto& GetThreadData() { return m_ThreadData; }

    void AddSwapchainResizeCallback(const std::function<void()>& resizeCallback);

  private:
    VkInstance m_Instance                     = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface                    = VK_NULL_HANDLE;

    Scoped<VulkanDevice> m_Device                           = nullptr;
    Scoped<VulkanAllocator> m_Allocator                     = nullptr;
    Scoped<VulkanSwapchain> m_Swapchain                     = nullptr;
    Scoped<VulkanCommandPool> m_TransferCommandPool         = nullptr;
    Scoped<VulkanCommandPool> m_GraphicsCommandPool         = nullptr;
    Scoped<VulkanDescriptorAllocator> m_DescriptorAllocator = nullptr;

    VulkanCommandBuffer* m_CurrentCommandBuffer = nullptr;

    // Sync objects GPU-GPU.
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    std::vector<VkSemaphore> m_ImageAcquiredSemaphores;

    // Sync objects CPU-GPU
    std::vector<VkFence> m_InFlightFences;
    float m_LastGPUWaitTime = 0.0f;

    // Multithreaded stuff
    struct ThreadData
    {
        // TODO: If it makes sense, implement query pool
        Scoped<VulkanCommandPool> CommandPool = nullptr;
        std::vector<VulkanCommandBuffer> SecondaryShadowMapCommandBuffers;  // Per-frame
        std::vector<VulkanCommandBuffer> SecondaryGeometryCommandBuffers;   // Per-frame
    };
    std::vector<ThreadData> m_ThreadData;

    void CreateInstance();
    void CreateDebugMessenger();
    void CreateSurface();
    void CreateSyncObjects();
    void InitializeMultithreadedRenderer();

    bool CheckVulkanAPISupport() const;
    bool CheckValidationLayerSupport();
    const std::vector<const char*> GetRequiredExtensions();
};

}  // namespace Gauntlet
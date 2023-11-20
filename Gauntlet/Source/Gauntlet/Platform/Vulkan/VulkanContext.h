#pragma once

#include "Gauntlet/Core/Core.h"
#include "Gauntlet/Renderer/GraphicsContext.h"

#include <volk/volk.h>

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

    void WaitDeviceOnFinish() final override;
    uint32_t GetCurrentFrameIndex() const final override;

    FORCEINLINE const auto& GetUploadFence() const { return m_UploadFence; }
    FORCEINLINE auto& GetUploadFence() { return m_UploadFence; }

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

    void AddSwapchainResizeCallback(const std::function<void()>& resizeCallback);

    // TODO: Work with queries
    FORCEINLINE const std::vector<uint64_t>& GetPipelineStats() const final override { return s_PipelineStats; }
    FORCEINLINE const std::vector<std::string>& GetPipelineStatNames() const final override { return s_PipelineStatNames; }

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

    // For single-time commands
    VkFence m_UploadFence = VK_NULL_HANDLE;

    // TODO: Move them to Renderer && abstract
    // Query Statistics
    VkQueryPool m_QueryPool = VK_NULL_HANDLE;

    static inline std::vector<uint64_t> s_PipelineStats;
    static inline std::vector<std::string> s_PipelineStatNames;

    void CreateInstance();
    void CreateDebugMessenger();
    void CreateSurface();
    void CreateSyncObjects();

    bool CheckVulkanAPISupport() const;
    bool CheckValidationLayerSupport();
    const std::vector<const char*> GetRequiredExtensions();
};

}  // namespace Gauntlet
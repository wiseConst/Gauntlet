#pragma once

#include "Eclipse/Core/Core.h"
#include "Eclipse/Renderer/GraphicsContext.h"

#include <volk/volk.h>
#include <glm/glm.hpp>

namespace Eclipse
{

class VulkanDevice;
class VulkanAllocator;
class VulkanSwapchain;
class VulkanCommandPool;

class VulkanRenderPass;
class VulkanPipeline;

class VulkanContext final : public GraphicsContext
{
  public:
    VulkanContext() = delete;
    VulkanContext(Scoped<Window>& InWindow);
    ~VulkanContext() = default;

    void BeginRender() final override;
    void EndRender() final override;

    void SwapBuffers() final override;
    void SetVSync(bool IsVSync) final override;
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

    FORCEINLINE const auto& GetGlobalRenderPass() const { return m_GlobalRenderPass; }
    FORCEINLINE auto& GetGlobalRenderPass() { return m_GlobalRenderPass; }

    FORCEINLINE VkBool32 IsDestroying() const { return m_bIsDestroying; }

    FORCEINLINE void SetClearColor(const glm::vec4& InColor) { ClearColor = {InColor.r, InColor.g, InColor.b, InColor.a}; }

    FORCEINLINE void AddPipelineToRebuild(const Ref<VulkanPipeline>& InPipeline) { m_PipelinesToRebuild.emplace_back(InPipeline); }

  private:
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

    Scoped<VulkanDevice> m_Device;
    Scoped<VulkanAllocator> m_Allocator;
    Scoped<VulkanSwapchain> m_Swapchain;
    Scoped<VulkanCommandPool> m_TransferCommandPool;
    Scoped<VulkanCommandPool> m_GraphicsCommandPool;

    VkBool32 m_bIsDestroying{VK_FALSE};
    VkClearColorValue ClearColor = {0.1f, 0.1f, 0.1f, 1.0};

    Scoped<VulkanRenderPass> m_GlobalRenderPass;  // Tied with Swapchain.

    // Sync objects GPU-GPU.
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    std::vector<VkSemaphore> m_ImageAcquiredSemaphores;

    // Sync objects CPU-GPU
    std::vector<VkFence> m_InFlightFences;

    std::vector<Ref<VulkanPipeline>> m_PipelinesToRebuild;

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
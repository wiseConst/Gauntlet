#pragma once

#include "Gauntlet/ImGui/ImGuiLayer.h"

#include "volk/volk.h"
#include "VulkanCommandBuffer.h"

namespace Gauntlet
{

class VulkanCommandPool;
class VulkanContext;

class VulkanImGuiLayer final : public ImGuiLayer
{
  public:
    VulkanImGuiLayer();
    ~VulkanImGuiLayer() = default;

    void OnAttach() final override;
    void OnDetach() final override;

    void BeginRender() final override;
    void EndRender() final override;

    void OnImGuiRender() final override {}

    FORCEINLINE const glm::vec2 GetViewportSize() const final override { return glm::vec2(m_ViewportSize.x, m_ViewportSize.y); }

  private:
    VulkanContext& m_Context;
    VulkanCommandBuffer* m_CurrentCommandBuffer;
    std::vector<VkFence> m_InFlightFences;

    VkDescriptorPool m_ImGuiDescriptorPool = VK_NULL_HANDLE;
    Ref<VulkanCommandPool> m_ImGuiCommandPool;

    std::vector<VkDescriptorSet> m_TextureIDs;
    ImFont* m_DefaultFont = nullptr;

    ImVec2 m_ViewportSize = ImVec2(0.0f, 0.0f);

    void CreateSyncObjects();
};
}  // namespace Gauntlet
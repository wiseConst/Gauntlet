#pragma once

#include "Eclipse/ImGui/ImGuiLayer.h"

#include "volk/volk.h"

namespace Eclipse
{

class VulkanCommandPool;
class VulkanFramebuffer;

class VulkanImGuiLayer final : public ImGuiLayer
{
  public:
    VulkanImGuiLayer() : ImGuiLayer("ImGuiLayer"), m_ImGuiCommandPool(nullptr) {}
    ~VulkanImGuiLayer() = default;

    void OnAttach() final override;
    void OnDetach() final override;

    void BeginRender() final override;
    void EndRender() final override;

    void OnImGuiRender() final override {}

    FORCEINLINE const glm::vec2 GetViewportSize() const final override { return glm::vec2(m_ViewportSize.x, m_ViewportSize.y); }

  private:
    VkDescriptorPool m_ImGuiDescriptorPool = VK_NULL_HANDLE;

    Ref<VulkanCommandPool> m_ImGuiCommandPool;
    Ref<VulkanFramebuffer> m_Framebuffer;

    std::vector<VkDescriptorSet> m_TextureIDs;
    ImFont* m_DefaultFont = nullptr;

    ImVec2 m_ViewportSize = ImVec2(0.0f, 0.0f);

    void SetStyle();
};
}  // namespace Eclipse
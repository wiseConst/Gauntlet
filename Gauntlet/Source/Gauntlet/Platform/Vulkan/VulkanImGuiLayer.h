#pragma once

#include "Gauntlet/ImGui/ImGuiLayer.h"

#include "volk/volk.h"

namespace Gauntlet
{

class VulkanContext;
class VulkanCommandBuffer;

class VulkanImGuiLayer final : public ImGuiLayer
{
  public:
    VulkanImGuiLayer();
    ~VulkanImGuiLayer() = default;

    void OnAttach() final override;
    void OnDetach() final override;

    void BeginRender() final override;
    void EndRender() final override;

    void OnUpdate(const float deltaTime) final override {}
    void OnEvent(Event& event) final override;
    void OnImGuiRender() final override {}

    void BlockEvents(const bool bBlockEvents) final override { m_bBlockEvents = bBlockEvents; }

  private:
    VulkanContext& m_Context;
    VulkanCommandBuffer* m_CurrentCommandBuffer;

    VkDescriptorPool m_ImGuiDescriptorPool = VK_NULL_HANDLE;
    bool m_bBlockEvents{false};

    void SetCustomUIStyle();
};
}  // namespace Gauntlet
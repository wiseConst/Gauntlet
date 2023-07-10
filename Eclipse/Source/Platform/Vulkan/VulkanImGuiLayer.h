#pragma once

#include "Eclipse/ImGui/ImGuiLayer.h"

#include "volk/volk.h"
#include "VulkanRenderPass.h"
#include "VulkanCommandPool.h"

namespace Eclipse
{
class VulkanCommandPool;

class VulkanImGuiLayer final : public ImGuiLayer
{
  public:
    VulkanImGuiLayer() : ImGuiLayer("ImGuiLayer"){};
    ~VulkanImGuiLayer() = default;

    void OnAttach() final override;
    void OnDetach() final override;

    void BeginRender() final override;
    void EndRender() final override;

    void OnImGuiRender() final override {}

  private:
    VkDescriptorPool m_ImGuiDescriptorPool = VK_NULL_HANDLE;
    Scoped<VulkanCommandPool> m_ImGuiCommandPool;
    Scoped<VulkanRenderPass> m_ImGuiRenderPass;

    void SetStyle();
};
}  // namespace Eclipse
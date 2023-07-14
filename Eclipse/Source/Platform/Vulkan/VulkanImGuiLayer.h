#pragma once

#include "Eclipse/ImGui/ImGuiLayer.h"

#include "volk/volk.h"

#include "VulkanRenderPass.h"
#include "VulkanCommandPool.h"

namespace Eclipse
{

// Forward decls compile error
//class VulkanCommandPool;
//class VulkanRenderPass;

class VulkanImGuiLayer final : public ImGuiLayer
{
  public:
    VulkanImGuiLayer() : ImGuiLayer("ImGuiLayer"), m_ImGuiCommandPool(nullptr), m_ImGuiRenderPass(nullptr){};
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

    ImFont* m_DefaultFont = nullptr;

    void SetStyle();
};
}  // namespace Eclipse
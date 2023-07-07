#pragma once

#include "Eclipse/ImGui/ImGuiLayer.h"

#include "volk/volk.h"

namespace Eclipse
{

class VulkanImGuiLayer final : public ImGuiLayer
{
  public:
    VulkanImGuiLayer() : ImGuiLayer("ImGuiLayer"){};
    ~VulkanImGuiLayer() = default;

    void OnAttach() final override;
    void OnDetach() final override;

    void BeginRender() final override;
    void EndRender() final override;

  private:
    VkDescriptorPool m_ImGuiDescriptorPool = VK_NULL_HANDLE;

    void SetStyle();
};
}  // namespace Eclipse
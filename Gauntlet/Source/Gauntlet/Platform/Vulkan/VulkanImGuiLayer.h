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

  private:
    VulkanContext& m_Context;
    VulkanCommandBuffer* m_CurrentCommandBuffer;
    std::vector<VkFence> m_InFlightFences;

    VkDescriptorPool m_ImGuiDescriptorPool = VK_NULL_HANDLE;
    Ref<VulkanCommandPool> m_ImGuiCommandPool;

    ImFont* m_DefaultFont = nullptr;

    void CreateSyncObjects();
};
}  // namespace Gauntlet
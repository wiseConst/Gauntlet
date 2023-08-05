#pragma once

#include "Eclipse/Renderer/Framebuffer.h"

#include <volk/volk.h>

namespace Eclipse
{

class VulkanImage;
class VulkanRenderPass;

class VulkanFramebuffer final : public Framebuffer
{
  public:
    VulkanFramebuffer(const FramebufferSpecification& InFramebufferSpecification);
    ~VulkanFramebuffer() = default;

    FORCEINLINE const auto& GetRenderPass() const { return m_RenderPass; }

    FORCEINLINE const auto& GetColorAttachments() const { return m_ColorAttachments; }
    FORCEINLINE const auto& GetDepthAttachment() const { return m_DepthAttachment; }

    FORCEINLINE void SetClearColor(const glm::vec4& InClearColor) { m_Specification.ClearColor = InClearColor; }

    FORCEINLINE const auto& GetClearValues() { return m_ClearValues; }

    void BeginRenderPass(const VkCommandBuffer& InCommandBuffer);
    FORCEINLINE void EndRenderPass(const VkCommandBuffer& InCommandBuffer) { vkCmdEndRenderPass(InCommandBuffer); }

    void InvalidateRenderPass();

    void Destroy() final override;

    FORCEINLINE virtual FramebufferSpecification& GetSpec() final override { return m_Specification; }

  private:
    FramebufferSpecification m_Specification;
    std::vector<VkClearValue> m_ClearValues;

    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_Framebuffers;

    std::vector<Ref<VulkanImage>> m_ColorAttachments;
    Ref<VulkanImage> m_DepthAttachment;

    void Invalidate();

    void DestroyRenderPass();
};
}  // namespace Eclipse
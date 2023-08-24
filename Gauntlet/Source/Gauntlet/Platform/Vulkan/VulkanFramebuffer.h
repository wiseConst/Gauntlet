#pragma once

#include "Gauntlet/Renderer/Framebuffer.h"

#include <volk/volk.h>

namespace Gauntlet
{

class VulkanImage;
class VulkanRenderPass;

class VulkanFramebuffer final : public Framebuffer
{
  public:
    VulkanFramebuffer(const FramebufferSpecification& framebufferSpecification);
    ~VulkanFramebuffer() = default;

    FORCEINLINE const auto& GetRenderPass() const { return m_RenderPass; }
    FORCEINLINE const auto& GetColorAttachments() const { return m_ColorAttachments; }
    FORCEINLINE const auto& GetDepthAttachment() const { return m_DepthAttachment; }
    FORCEINLINE FramebufferSpecification& GetSpec() final override { return m_Specification; }

    FORCEINLINE void SetClearColor(const glm::vec4& clearColor) { m_Specification.ClearColor = clearColor; }

    void BeginRenderPass(const VkCommandBuffer& commandBuffer);
    FORCEINLINE void EndRenderPass(const VkCommandBuffer& commandBuffer) { vkCmdEndRenderPass(commandBuffer); }

    FORCEINLINE void Resize(uint32_t width, uint32_t height)
    {
        m_Specification.Width  = width;
        m_Specification.Height = height;
        Invalidate();
        LOG_INFO("Framebuffer <%s> resized (%u, %u).", m_Specification.Name.data(), width, height);
    }

    void Invalidate();
    void Destroy() final override;

    const uint32_t GetWidth() const final override;
    const uint32_t GetHeight() const final override;

  private:
    FramebufferSpecification m_Specification;
    std::array<VkClearValue, 2> m_ClearValues;  // First one is for color, the second is for depth/stencil

    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_Framebuffers;

    std::vector<Ref<VulkanImage>> m_ColorAttachments;
    Ref<VulkanImage> m_DepthAttachment;
};
}  // namespace Gauntlet
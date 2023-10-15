#pragma once

#include "Gauntlet/Renderer/Framebuffer.h"

#include "VulkanUtility.h"
#include <volk/volk.h>

namespace Gauntlet
{

class VulkanImage;

class VulkanFramebuffer final : public Framebuffer
{
  public:
    VulkanFramebuffer(const FramebufferSpecification& framebufferSpecification);
    ~VulkanFramebuffer() = default;

    FORCEINLINE const auto& GetRenderPass() const { return m_RenderPass; }
    FORCEINLINE const std::vector<FramebufferAttachment>& GetAttachments() const final override { return m_Attachments; }
    const Ref<VulkanImage> GetDepthAttachment() const;
    FORCEINLINE FramebufferSpecification& GetSpecification() final override { return m_Specification; }
    const VkFramebuffer& Get() const;

    void SetDepthStencilClearColor(const float depth, const uint32_t stencil) final override;

    void BeginRenderPass(const VkCommandBuffer& commandBuffer, const VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE);
    FORCEINLINE void EndRenderPass(const VkCommandBuffer& commandBuffer) { vkCmdEndRenderPass(commandBuffer); }

    FORCEINLINE void Resize(uint32_t width, uint32_t height)
    {
        m_Specification.Width  = width;
        m_Specification.Height = height;
        Invalidate();
    }

    void Invalidate();
    void Destroy() final override;

    FORCEINLINE const uint32_t GetWidth() const final override { return m_Specification.Width; }
    FORCEINLINE const uint32_t GetHeight() const final override { return m_Specification.Height; }

  private:
    FramebufferSpecification m_Specification;
    std::vector<VkClearValue> m_ClearValues;

    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_Framebuffers;  // Per-frame

    std::vector<FramebufferAttachment> m_Attachments;

    // Called only once when creating.
    void Create();
};
}  // namespace Gauntlet
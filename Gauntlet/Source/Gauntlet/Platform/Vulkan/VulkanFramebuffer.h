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
    VulkanFramebuffer()  = delete;
    ~VulkanFramebuffer() = default;

    FORCEINLINE const std::vector<FramebufferAttachment>& GetAttachments() const final override { return m_Attachments; }
    const Ref<VulkanImage> GetDepthAttachment() const;

    FORCEINLINE FramebufferSpecification& GetSpecification() final override { return m_Specification; }

    void BeginPass(const Ref<CommandBuffer>& commandBuffer) final override;
    void EndPass(const Ref<CommandBuffer>& commandBuffer) final override;

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
    std::vector<VkRenderingAttachmentInfo> m_AttachmentInfos;
    VkRenderingAttachmentInfo m_DepthStencilAttachmentInfo;

    std::vector<FramebufferAttachment> m_Attachments;
};
}  // namespace Gauntlet
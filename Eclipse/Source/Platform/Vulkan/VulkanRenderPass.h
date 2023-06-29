#pragma once

#include "Eclipse/Core/Core.h"
#include <volk/volk.h>

namespace Eclipse
{

struct RenderPassSpecification
{
  public:
    RenderPassSpecification() {}
    ~RenderPassSpecification() = default;

    RenderPassSpecification(const RenderPassSpecification& that)
    {
        Attachments = that.Attachments;
        AttachmentRefs = that.AttachmentRefs;
        Subpasses = that.Subpasses;
        Dependencies = that.Dependencies;
        DepthImageView = that.DepthImageView;
    }

    std::vector<VkAttachmentDescription> Attachments;
    std::vector<VkAttachmentReference> AttachmentRefs;

    std::vector<VkSubpassDescription> Subpasses;
    std::vector<VkSubpassDependency> Dependencies;

    VkImageView DepthImageView = VK_NULL_HANDLE;
};

/* RenderPass
 * In Vulkan, all of the rendering happens inside a VkRenderPass.
 * It is not possible to do rendering commands outside of a renderpass,
 * but it is possible to do Compute commands without them.
 *
 * A VkRenderPass is a Vulkan object that encapsulates the state needed to setup the “target” for rendering,
 * and the state of the images you will be rendering to.
 *
 * A Renderpass will render into a Framebuffer.
 * The framebuffer links to the images you will render to, and it’s used when starting a renderpass to set the target images for rendering.
 *
 * A renderpass also contains subpasses, which are a bit like the rendering “steps”. They can be very useful in mobile GPUs,
 * as they allow the driver to optimize things quite a lot. In desktop GPUs, they are less important, so we aren’t going to use them.
 * When creating the renderpass, it will only have 1 subpass which is the minimum required for rendering.
 *
 * A very important thing that the renderpass does, is that it performs image layout changes when entering and exiting the renderpass.
 *
 * Images in the GPU aren’t necessarily in the format you would expect. For optimization purposes, the GPUs perform a lot of transformation
 * and reshuffling of them into internal opaque formats. For example, some GPUs will compress textures whenever they can, and will reorder
 * the way the pixels are arranged so that they mipmap better. In Vulkan, you don’t have control of that, but there is control over the
 * layout for the image, which lets the driver transform the image to those optimized internal formats.
 */
class VulkanRenderPass final : private Uncopyable, private Unmovable
{
  public:
    VulkanRenderPass(const RenderPassSpecification& InRenderPassSpecification);
    VulkanRenderPass() = delete;
    ~VulkanRenderPass() = default;

    void Begin(const VkCommandBuffer& InCommandBuffer, const std::vector<VkClearValue>& InClearValues) const;
    FORCEINLINE void End(const VkCommandBuffer& InCommandBuffer) const { vkCmdEndRenderPass(InCommandBuffer); }

    FORCEINLINE const auto& Get() const { return m_RenderPass; }
    FORCEINLINE auto& Get() { return m_RenderPass; }

    FORCEINLINE const auto& GetFramebuffer(const uint32_t Index) const { return m_Framebuffers[Index]; }
    FORCEINLINE auto& GetFramebuffer(const uint32_t Index) { return m_Framebuffers[Index]; }

    void Destroy();

  private:
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    RenderPassSpecification m_RenderPassSpecification;
    std::vector<VkFramebuffer> m_Framebuffers;

    void CreateFramebuffers();
};

}  // namespace Eclipse
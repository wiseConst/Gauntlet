#include "EclipsePCH.h"
#include "VulkanRenderPass.h"

#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"

namespace Eclipse
{
VulkanRenderPass::VulkanRenderPass(const RenderPassSpecification& InRenderPassSpecification)
    : m_RenderPassSpecification(InRenderPassSpecification)
{
    Invalidate();
}

void VulkanRenderPass::Invalidate()
{
    if (m_RenderPass != VK_NULL_HANDLE)
    {
        Destroy();
    }

    auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    std::vector<VkSubpassDescription> Subpasses(m_RenderPassSpecification.Subpasses.size());
    Subpasses[0].pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    Subpasses[0].colorAttachmentCount = 1;

    VkAttachmentReference ColorAttachmentRef = {};
    ColorAttachmentRef.attachment            = 0;
    ColorAttachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    Subpasses[0].pColorAttachments           = &ColorAttachmentRef;

    VkAttachmentReference DepthAttachmentRef = {};
    if (m_RenderPassSpecification.Subpasses[0].pDepthStencilAttachment)
    {
        DepthAttachmentRef.attachment = 1;
        DepthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        Subpasses[0].pDepthStencilAttachment = &DepthAttachmentRef;
    }
    m_RenderPassSpecification.Subpasses = Subpasses;

    VkRenderPassCreateInfo RenderPassCreateInfo = {};
    RenderPassCreateInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassCreateInfo.attachmentCount        = static_cast<uint32_t>(m_RenderPassSpecification.Attachments.size());
    RenderPassCreateInfo.pAttachments           = m_RenderPassSpecification.Attachments.data();
    RenderPassCreateInfo.dependencyCount        = static_cast<uint32_t>(m_RenderPassSpecification.Dependencies.size());
    RenderPassCreateInfo.pDependencies          = m_RenderPassSpecification.Dependencies.data();
    RenderPassCreateInfo.subpassCount           = static_cast<uint32_t>(m_RenderPassSpecification.Subpasses.size());  // ONLY 1 RN
    RenderPassCreateInfo.pSubpasses             = m_RenderPassSpecification.Subpasses.data();

    VK_CHECK(vkCreateRenderPass(Context.GetDevice()->GetLogicalDevice(), &RenderPassCreateInfo, nullptr, &m_RenderPass),
             "Failed to create render pass!");

    CreateFramebuffers();
}

void VulkanRenderPass::Begin(const VkCommandBuffer& InCommandBuffer, const std::vector<VkClearValue>& InClearValues) const
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetSwapchain()->IsValid(), "Vulkan swapchain is not valid!");

    VkRenderPassBeginInfo RenderPassBeginInfo = {};
    RenderPassBeginInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RenderPassBeginInfo.clearValueCount       = static_cast<uint32_t>(InClearValues.size());
    RenderPassBeginInfo.pClearValues          = InClearValues.data();
    RenderPassBeginInfo.renderPass            = m_RenderPass;
    RenderPassBeginInfo.framebuffer =
        m_Framebuffers[Context.GetSwapchain()->GetCurrentImageIndex()];  // What image we will render into for this renderpass.
    RenderPassBeginInfo.renderArea.extent = Context.GetSwapchain()->GetImageExtent();
    RenderPassBeginInfo.renderArea.offset = {0, 0};  // TEST WITH OTHER VALUES

    vkCmdBeginRenderPass(InCommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanRenderPass::CreateFramebuffers()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");
    ELS_ASSERT(Context.GetSwapchain()->IsValid(), "Vulkan swapchain is not valid!");
    m_Framebuffers.resize(Context.GetSwapchain()->GetImageCount());

    std::vector<VkImageView> Attachments(1);
    if (m_RenderPassSpecification.DepthImageView)
    {
        m_RenderPassSpecification.DepthImageView = Context.GetSwapchain()->GetDepthImageView();
        Attachments.push_back(m_RenderPassSpecification.DepthImageView);
    }

    // VkFrameBuffer is what maps attachments to a renderpass. That's really all it is.
    VkFramebufferCreateInfo FramebufferCreateInfo = {};
    FramebufferCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FramebufferCreateInfo.renderPass              = m_RenderPass;
    FramebufferCreateInfo.width                   = Context.GetSwapchain()->GetImageExtent().width;
    FramebufferCreateInfo.height                  = Context.GetSwapchain()->GetImageExtent().height;
    FramebufferCreateInfo.attachmentCount         = static_cast<uint32_t>(Attachments.size());
    FramebufferCreateInfo.pAttachments            = Attachments.data();
    FramebufferCreateInfo.layers                  = 1;

    for (size_t i = 0; i < m_Framebuffers.size(); ++i)
    {
        Attachments[0] = Context.GetSwapchain()->GetImageViews()[i];
        VK_CHECK(vkCreateFramebuffer(Context.GetDevice()->GetLogicalDevice(), &FramebufferCreateInfo, nullptr, &m_Framebuffers[i]),
                 "Failed to create a framebuffer!");
    }
}

void VulkanRenderPass::Destroy()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    for (auto& Framebuffer : m_Framebuffers)
    {
        vkDestroyFramebuffer(Context.GetDevice()->GetLogicalDevice(), Framebuffer, nullptr);
    }

    vkDestroyRenderPass(Context.GetDevice()->GetLogicalDevice(), m_RenderPass, nullptr);
}

}  // namespace Eclipse
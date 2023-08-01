#include "EclipsePCH.h"
#include "VulkanFramebuffer.h"

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanImage.h"
#include "VulkanDevice.h"

namespace Eclipse
{
static VkAttachmentLoadOp EclipseLoadOpToVulkan(ELoadOp InLoadOp)
{
    switch (InLoadOp)
    {
        case ELoadOp::CLEAR: return VK_ATTACHMENT_LOAD_OP_CLEAR;
        case ELoadOp::DONT_CARE: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        case ELoadOp::LOAD: return VK_ATTACHMENT_LOAD_OP_LOAD;
    }

    ELS_ASSERT(false, "Unknown load op!");
    return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
}

static VkAttachmentStoreOp EclipseStoreOpToVulkan(EStoreOp InStoreOp)
{
    switch (InStoreOp)
    {
        case EStoreOp::DONT_CARE: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        case EStoreOp::STORE: return VK_ATTACHMENT_STORE_OP_STORE;
    }

    ELS_ASSERT(false, "Unknown load op!");
    return VK_ATTACHMENT_STORE_OP_DONT_CARE;
}

VulkanFramebuffer::VulkanFramebuffer(const FramebufferSpecification& InFramebufferSpecification)
    : m_Specification(InFramebufferSpecification)
{
    InvalidateRenderPass();
}

void VulkanFramebuffer::BeginRenderPass(const VkCommandBuffer& InCommandBuffer)
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetSwapchain()->IsValid(), "Vulkan swapchain is not valid!");

    m_ClearValues.resize(1);
    m_ClearValues[0].color = {m_Specification.ClearColor.r, m_Specification.ClearColor.g, m_Specification.ClearColor.b,
                              m_Specification.ClearColor.a};

    if (m_Specification.bHasDepth)
    {
        VkClearDepthStencilValue DepthStencil = {1.0f, 0};

        VkClearValue DepthColor = {};
        DepthColor.depthStencil = DepthStencil;
        m_ClearValues.push_back(DepthColor);
    }

    VkRenderPassBeginInfo RenderPassBeginInfo = {};
    RenderPassBeginInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RenderPassBeginInfo.framebuffer           = m_Framebuffers[Context.GetSwapchain()->GetCurrentImageIndex()];
    RenderPassBeginInfo.pClearValues          = m_ClearValues.data();
    RenderPassBeginInfo.clearValueCount       = static_cast<uint32_t>(m_ClearValues.size());
    RenderPassBeginInfo.renderPass            = m_RenderPass;

    const auto ImageExtent         = m_Specification.bIsSwapchainTarget ? Context.GetSwapchain()->GetImageExtent()
                                                                        : VkExtent2D{m_Specification.Width, m_Specification.Height};
    VkRect2D RenderArea            = {};
    RenderArea.offset              = {0, 0};
    RenderArea.extent              = ImageExtent;
    RenderPassBeginInfo.renderArea = RenderArea;

    vkCmdBeginRenderPass(InCommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanFramebuffer::InvalidateRenderPass()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    if (!m_Specification.bIsSwapchainTarget) Invalidate();

    const auto& Swapchain = Context.GetSwapchain();
    if (m_RenderPass) DestroyRenderPass();

    // RenderPass creation
    {
        std::vector<VkAttachmentDescription> Attachments(1);
        // Color Attachment
        Attachments[0].format  = m_Specification.bIsSwapchainTarget ? Swapchain->GetImageFormat() : m_ColorAttachments[0]->GetFormat();
        Attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        Attachments[0].initialLayout = m_Specification.bIsSwapchainTarget ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_UNDEFINED;
        Attachments[0].finalLayout =
            m_Specification.bIsSwapchainTarget ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        Attachments[0].loadOp         = EclipseLoadOpToVulkan(m_Specification.LoadOp);
        Attachments[0].storeOp        = EclipseStoreOpToVulkan(m_Specification.StoreOp);
        Attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        Attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        if (m_Specification.bHasDepth)
        {
            // Depth Attachment
            VkAttachmentDescription DepthAttachment = {};
            DepthAttachment.format  = m_Specification.bIsSwapchainTarget ? Swapchain->GetDepthFormat() : m_DepthAttachment->GetFormat();
            DepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            DepthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            DepthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            DepthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            DepthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            DepthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            DepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

            Attachments.emplace_back(DepthAttachment);
        }

        std::vector<VkAttachmentReference> AttachmentRefs = {{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};

        std::vector<VkSubpassDescription> Subpasses(1);
        Subpasses[0].pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        Subpasses[0].colorAttachmentCount = 1;

        if (m_Specification.bHasDepth)
        {
            VkAttachmentReference DepthAttachmentRef = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

            AttachmentRefs.emplace_back(DepthAttachmentRef);
            Subpasses[0].pDepthStencilAttachment = &AttachmentRefs[1];
        }
        Subpasses[0].pColorAttachments = &AttachmentRefs[0];

        std::vector<VkSubpassDependency> Dependencies(1);
        Dependencies[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
        Dependencies[0].dstSubpass    = 0;
        Dependencies[0].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        Dependencies[0].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        Dependencies[0].srcAccessMask = 0;
        Dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        if (m_Specification.bHasDepth)
        {
            VkSubpassDependency DepthDependency = {};
            DepthDependency.srcSubpass          = 0;
            DepthDependency.dstSubpass          = VK_SUBPASS_EXTERNAL;
            DepthDependency.srcStageMask        = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            DepthDependency.dstStageMask        = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            DepthDependency.srcAccessMask       = 0;
            DepthDependency.dstAccessMask       = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            Dependencies.push_back(DepthDependency);
        }

        VkRenderPassCreateInfo RenderPassCreateInfo = {};
        RenderPassCreateInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        RenderPassCreateInfo.attachmentCount        = static_cast<uint32_t>(Attachments.size());
        RenderPassCreateInfo.pAttachments           = Attachments.data();
        RenderPassCreateInfo.dependencyCount        = static_cast<uint32_t>(Dependencies.size());
        RenderPassCreateInfo.pDependencies          = Dependencies.data();
        RenderPassCreateInfo.pSubpasses             = Subpasses.data();
        RenderPassCreateInfo.subpassCount           = static_cast<uint32_t>(Subpasses.size());

        VK_CHECK(vkCreateRenderPass(Context.GetDevice()->GetLogicalDevice(), &RenderPassCreateInfo, nullptr, &m_RenderPass),
                 "Failed to create render pass!");
    }

    // Framebuffers creation
    VkImageView Attachments[2];
    if (m_Specification.bHasDepth)
        Attachments[1] = m_Specification.bIsSwapchainTarget ? Swapchain->GetDepthImageView() : m_DepthAttachment->GetView();

    const auto ImageExtent =
        m_Specification.bIsSwapchainTarget ? Swapchain->GetImageExtent() : VkExtent2D{m_Specification.Width, m_Specification.Height};

    VkFramebufferCreateInfo FramebufferCreateInfo = {};
    FramebufferCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FramebufferCreateInfo.renderPass              = m_RenderPass;
    FramebufferCreateInfo.layers                  = 1;
    FramebufferCreateInfo.pAttachments            = Attachments;
    FramebufferCreateInfo.attachmentCount         = m_Specification.bHasDepth ? 2 : 1;
    FramebufferCreateInfo.height                  = ImageExtent.height;
    FramebufferCreateInfo.width                   = ImageExtent.width;

    m_Framebuffers.resize(Swapchain->GetImageCount());
    for (uint32_t i = 0; i < m_Framebuffers.size(); ++i)
    {
        Attachments[0] = m_Specification.bIsSwapchainTarget ? Swapchain->GetImageViews()[i] : m_ColorAttachments[i]->GetView();
        VK_CHECK(vkCreateFramebuffer(Context.GetDevice()->GetLogicalDevice(), &FramebufferCreateInfo, nullptr, &m_Framebuffers[i]),
                 "Failed to create framebuffer!");
    }
}

void VulkanFramebuffer::Destroy()
{
    for (auto& ColorAttachment : m_ColorAttachments)
        ColorAttachment->Destroy();

    if (m_Specification.bHasDepth) m_DepthAttachment->Destroy();

    DestroyRenderPass();
}

void VulkanFramebuffer::Invalidate()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetSwapchain()->IsValid(), "Vulkan swapchain is not valid!");

    const auto& Swapchain = Context.GetSwapchain();
    const auto ImageExtent =
        m_Specification.bIsSwapchainTarget ? Swapchain->GetImageExtent() : VkExtent2D{m_Specification.Width, m_Specification.Height};

    if (m_ColorAttachments.size() == 0)
    {
        for (uint32_t i = 0; i < Swapchain->GetImageCount(); ++i)
        {
            ImageSpecification ImageSpec = {};
            ImageSpec.Width              = ImageExtent.width;
            ImageSpec.Height             = ImageExtent.height;
            ImageSpec.Format             = EImageFormat::RGBA;
            ImageSpec.Usage              = EImageUsage::Attachment;

            m_ColorAttachments.emplace_back(new VulkanImage(ImageSpec));
        }

        if (m_Specification.bHasDepth)
        {
            ImageSpecification DepthImageSpec = {};
            DepthImageSpec.Width              = ImageExtent.width;
            DepthImageSpec.Height             = ImageExtent.height;
            DepthImageSpec.Format             = EImageFormat::DEPTH32F;
            DepthImageSpec.Usage              = EImageUsage::Attachment;

            m_DepthAttachment.reset(new VulkanImage(DepthImageSpec));
        }

        return;
    }

    for (uint32_t i = 0; i < Swapchain->GetImageCount(); ++i)
    {
        m_ColorAttachments[i]->Destroy();
        m_ColorAttachments[i]->SetExtent(ImageExtent);
        m_ColorAttachments[i]->Create();
    }

    if (m_Specification.bHasDepth)
    {
        m_DepthAttachment->Destroy();
        m_DepthAttachment->SetExtent(ImageExtent);
        m_DepthAttachment->Create();
    }
}

void VulkanFramebuffer::DestroyRenderPass()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetSwapchain()->IsValid(), "Vulkan swapchain is not valid!");

    vkDestroyRenderPass(Context.GetDevice()->GetLogicalDevice(), m_RenderPass, nullptr);

    for (auto& Framebuffer : m_Framebuffers)
        vkDestroyFramebuffer(Context.GetDevice()->GetLogicalDevice(), Framebuffer, nullptr);
}

}  // namespace Eclipse
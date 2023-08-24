#include "GauntletPCH.h"
#include "VulkanFramebuffer.h"

#include "VulkanUtility.h"
#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanImage.h"
#include "VulkanDevice.h"

#include "Gauntlet/Core/Application.h"
#include "Gauntlet/Core/Window.h"

namespace Gauntlet
{
static VkAttachmentLoadOp GauntletLoadOpToVulkan(ELoadOp loadOp)
{
    switch (loadOp)
    {
        case ELoadOp::CLEAR: return VK_ATTACHMENT_LOAD_OP_CLEAR;
        case ELoadOp::DONT_CARE: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        case ELoadOp::LOAD: return VK_ATTACHMENT_LOAD_OP_LOAD;
    }

    GNT_ASSERT(false, "Unknown load op!");
    return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
}

static VkAttachmentStoreOp GauntletStoreOpToVulkan(EStoreOp storeOp)
{
    switch (storeOp)
    {
        case EStoreOp::DONT_CARE: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        case EStoreOp::STORE: return VK_ATTACHMENT_STORE_OP_STORE;
    }

    GNT_ASSERT(false, "Unknown load op!");
    return VK_ATTACHMENT_STORE_OP_DONT_CARE;
}

VulkanFramebuffer::VulkanFramebuffer(const FramebufferSpecification& framebufferSpecification) : m_Specification(framebufferSpecification)
{
    if (m_Specification.Width == 0 || m_Specification.Height == 0)
    {
        m_Specification.Width  = Application::Get().GetWindow()->GetWidth();
        m_Specification.Height = Application::Get().GetWindow()->GetHeight();
    }

    Invalidate();
}

void VulkanFramebuffer::Invalidate()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(Context.GetSwapchain()->IsValid(), "Vulkan swapchain is not valid!");

    const auto& Swapchain     = Context.GetSwapchain();
    const auto& LogicalDevice = Context.GetDevice()->GetLogicalDevice();
    const bool bHasDepth      = m_Specification.DepthLoadOp != ELoadOp::DONT_CARE || m_Specification.DepthStoreOp != EStoreOp::DONT_CARE;

    if (m_ColorAttachments.empty())
    {
        for (uint32_t i = 0; i < Swapchain->GetImageCount(); ++i)
        {
            ImageSpecification ImageSpec = {};
            ImageSpec.Width              = m_Specification.Width;
            ImageSpec.Height             = m_Specification.Height;
            ImageSpec.Format             = EImageFormat::RGBA;
            ImageSpec.Usage              = EImageUsage::Attachment;
            ImageSpec.CreateTextureID    = true;

            m_ColorAttachments.emplace_back(new VulkanImage(ImageSpec));
        }

        if (bHasDepth)
        {
            ImageSpecification DepthImageSpec = {};
            DepthImageSpec.Width              = m_Specification.Width;
            DepthImageSpec.Height             = m_Specification.Height;
            DepthImageSpec.Format             = EImageFormat::DEPTH32F;
            DepthImageSpec.Usage              = EImageUsage::Attachment;
            DepthImageSpec.CreateTextureID    = false;  // TODO: Should I also craete texture id for depth image to display it sooner?

            m_DepthAttachment.reset(new VulkanImage(DepthImageSpec));
        }
    }

    if (m_RenderPass)
    {
        for (auto& Framebuffer : m_Framebuffers)
            vkDestroyFramebuffer(LogicalDevice, Framebuffer, nullptr);

        for (uint32_t i = 0; i < Swapchain->GetImageCount(); ++i)
        {
            m_ColorAttachments[i]->GetSpecification().Width  = m_Specification.Width;
            m_ColorAttachments[i]->GetSpecification().Height = m_Specification.Height;
            m_ColorAttachments[i]->Invalidate();
        }

        if (bHasDepth)
        {
            m_DepthAttachment->GetSpecification().Width  = m_Specification.Width;
            m_DepthAttachment->GetSpecification().Height = m_Specification.Height;
            m_DepthAttachment->Invalidate();
        }
    }

    // RenderPass creation
    if (!m_RenderPass)
    {
        std::vector<VkAttachmentDescription> Attachments(1);
        // Color Attachment
        Attachments[0].format         = m_ColorAttachments[0]->GetFormat();
        Attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
        Attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        Attachments[0].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        Attachments[0].loadOp         = GauntletLoadOpToVulkan(m_Specification.ColorLoadOp);
        Attachments[0].storeOp        = GauntletStoreOpToVulkan(m_Specification.ColorStoreOp);
        Attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        Attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        // Depth Attachment
        if (bHasDepth)
        {
            VkAttachmentDescription DepthAttachment = {};
            DepthAttachment.format                  = m_DepthAttachment->GetFormat();
            DepthAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
            DepthAttachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
            DepthAttachment.finalLayout             = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            DepthAttachment.loadOp                  = GauntletLoadOpToVulkan(m_Specification.DepthLoadOp);
            DepthAttachment.storeOp                 = GauntletStoreOpToVulkan(m_Specification.DepthStoreOp);
            DepthAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            DepthAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;

            Attachments.emplace_back(DepthAttachment);
        }

        std::vector<VkAttachmentReference> AttachmentRefs = {{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};

        VkSubpassDescription SubpassDesc = {};
        SubpassDesc.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        SubpassDesc.colorAttachmentCount = 1;

        if (bHasDepth)
        {
            VkAttachmentReference DepthAttachmentRef = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

            AttachmentRefs.emplace_back(DepthAttachmentRef);
            SubpassDesc.pDepthStencilAttachment = &AttachmentRefs[1];
        }
        SubpassDesc.pColorAttachments = &AttachmentRefs[0];

        /* Some explanation about subpass dependency
         * srcSubpass:
         * Index of subpass we depend on(current subpass won't start until srcSubpass has finished execution)
         * If we wanted to depend on a subpass that's part of a previous RenderPass, we could just pass in VK_SUBPASS_EXTERNAL here
         * instead. In this case, that would mean "wait for all of the subpasses within all of the render passes before this one".
         *
         * dstSubpass: Index to the current subpass, i.e. the one this dependency exists for. (Current subpass that starts)
         *
         * srcStageMask: Finish executing srcSubpass on this stage before we move onto current(dstSubpass).
         * dstStageMask: Is a bitmask of all of the Vulkan stages in dstSubpass that we're not allowed to execute until after the stages in
         * srcStageMask have completed within srcSubpass. IMO. This is the stage that your dstSubpass gonna execute only when srcSubpass
         * completed his srcStageMask.
         *
         * Okay, so, now we've specified the execution dependencies (the order in which these subpasses must execute) between our two
         * subpasses. But GPUs are complicated beasts that do a lot of caching of images and such, so just specifying the order we need
         * these rendering commands to occur in actually isn't enough. We also need to tell Vulkan the memory access types we need and when
         * we need them, so it can update caches and such accordingly.
         *
         * srcAccessMask: Is a bitmask of all the Vulkan memory access types used by srcSubpass
         * dstAccessMask: Is a bitmask of all the Vulkan memory access types we're going to use in dstSubpass.
         *
         * Think of it like we're saying: "after you've finished writing to the color attachment in srcSubpass, 'flush' the results as
         * needed for us to be able to read it in our shaders."
         *
         */
        std::vector<VkSubpassDependency> Dependencies(1);
        // Color dependency
        Dependencies[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
        Dependencies[0].dstSubpass    = 0;
        Dependencies[0].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        Dependencies[0].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        Dependencies[0].srcAccessMask = 0;
        Dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        if (bHasDepth)
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
        RenderPassCreateInfo.subpassCount           = 1;
        RenderPassCreateInfo.pSubpasses             = &SubpassDesc;

        VK_CHECK(vkCreateRenderPass(LogicalDevice, &RenderPassCreateInfo, nullptr, &m_RenderPass), "Failed to create render pass!");
    }

    // Framebuffers creation
    VkImageView Attachments[2];
    if (bHasDepth) Attachments[1] = m_DepthAttachment->GetView();

    VkFramebufferCreateInfo FramebufferCreateInfo = {};
    FramebufferCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FramebufferCreateInfo.renderPass              = m_RenderPass;
    FramebufferCreateInfo.layers                  = 1;
    FramebufferCreateInfo.pAttachments            = Attachments;
    FramebufferCreateInfo.attachmentCount         = bHasDepth ? 2 : 1;
    FramebufferCreateInfo.height                  = m_Specification.Height;
    FramebufferCreateInfo.width                   = m_Specification.Width;

    m_Framebuffers.resize(Swapchain->GetImageCount());
    for (uint32_t i = 0; i < m_Framebuffers.size(); ++i)
    {
        Attachments[0] = m_ColorAttachments[i]->GetView();
        VK_CHECK(vkCreateFramebuffer(Context.GetDevice()->GetLogicalDevice(), &FramebufferCreateInfo, nullptr, &m_Framebuffers[i]),
                 "Failed to create framebuffer!");
    }
}

void VulkanFramebuffer::BeginRenderPass(const VkCommandBuffer& commandBuffer)
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(Context.GetSwapchain()->IsValid(), "Vulkan swapchain is not valid!");

    m_ClearValues[0].color = {m_Specification.ClearColor.r, m_Specification.ClearColor.g, m_Specification.ClearColor.b,
                              m_Specification.ClearColor.a};

    if (m_DepthAttachment)
    {
        VkClearDepthStencilValue DepthStencil = {1.0f, 0};

        VkClearValue DepthColor = {};
        DepthColor.depthStencil = DepthStencil;
        m_ClearValues[1]        = DepthColor;
    }

    VkRenderPassBeginInfo RenderPassBeginInfo = {};
    RenderPassBeginInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RenderPassBeginInfo.framebuffer           = m_Framebuffers[Context.GetSwapchain()->GetCurrentImageIndex()];
    RenderPassBeginInfo.pClearValues          = m_ClearValues.data();
    RenderPassBeginInfo.clearValueCount       = m_DepthAttachment ? 2 : 1;
    RenderPassBeginInfo.renderPass            = m_RenderPass;

    VkRect2D RenderArea            = {};
    RenderArea.offset              = {0, 0};
    RenderArea.extent              = VkExtent2D{m_Specification.Width, m_Specification.Height};
    RenderPassBeginInfo.renderArea = RenderArea;

    vkCmdBeginRenderPass(commandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanFramebuffer::Destroy()
{
    for (auto& ColorAttachment : m_ColorAttachments)
        ColorAttachment->Destroy();

    if (m_DepthAttachment) m_DepthAttachment->Destroy();

    auto& Context           = (VulkanContext&)VulkanContext::Get();
    VkDevice& LogicalDevice = Context.GetDevice()->GetLogicalDevice();

    vkDestroyRenderPass(LogicalDevice, m_RenderPass, nullptr);

    for (auto& Framebuffer : m_Framebuffers)
        vkDestroyFramebuffer(LogicalDevice, Framebuffer, nullptr);
}

const uint32_t VulkanFramebuffer::GetWidth() const
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(Context.GetSwapchain()->IsValid(), "Vulkan swapchain is not valid!");

    if (m_ColorAttachments.size() == Context.GetSwapchain()->GetImageCount())
    {
        return m_ColorAttachments[Context.GetSwapchain()->GetCurrentImageIndex()]->GetWidth();
    }

    return m_ColorAttachments[0]->GetWidth();
}

const uint32_t VulkanFramebuffer::GetHeight() const
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(Context.GetSwapchain()->IsValid(), "Vulkan swapchain is not valid!");

    if (m_ColorAttachments.size() == Context.GetSwapchain()->GetImageCount())
    {
        return m_ColorAttachments[Context.GetSwapchain()->GetCurrentImageIndex()]->GetHeight();
    }

    return m_ColorAttachments[0]->GetHeight();
}

}  // namespace Gauntlet
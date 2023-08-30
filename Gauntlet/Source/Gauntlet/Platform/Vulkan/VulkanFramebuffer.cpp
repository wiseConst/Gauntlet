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

    const auto& swapchain     = Context.GetSwapchain();
    const auto& logicalDevice = Context.GetDevice()->GetLogicalDevice();

    FramebufferAttachmentSpecification framebufferColorAttachment = {};
    FramebufferAttachmentSpecification framebufferDepthAttachment = {};
    for (auto& attachment : m_Specification.Attachments)
    {
        GNT_ASSERT(m_Specification.AliveAttachments.empty(), "You got alive attachments and you want to create a new one?");
        if (!ImageUtils::IsDepthFormat(attachment.Format))
        {
            if (m_ColorAttachments.empty())
            {
                ImageSpecification ImageSpec = {};
                ImageSpec.Width              = m_Specification.Width;
                ImageSpec.Height             = m_Specification.Height;
                ImageSpec.Format             = attachment.Format;
                ImageSpec.Usage              = EImageUsage::Attachment;
                ImageSpec.CreateTextureID    = true;
                for (uint32_t i = 0; i < swapchain->GetImageCount(); ++i)
                    m_ColorAttachments.emplace_back(new VulkanImage(ImageSpec));
            }

            framebufferColorAttachment = attachment;
        }

        if (ImageUtils::IsDepthFormat(attachment.Format))
        {
            if (!m_DepthAttachment)
            {
                ImageSpecification DepthImageSpec = {};
                DepthImageSpec.Width              = m_Specification.Width;
                DepthImageSpec.Height             = m_Specification.Height;
                DepthImageSpec.Format             = attachment.Format;
                DepthImageSpec.Usage              = EImageUsage::Attachment;
                DepthImageSpec.CreateTextureID    = true;

                m_DepthAttachment.reset(new VulkanImage(DepthImageSpec));
            }

            framebufferDepthAttachment = attachment;
        }
    }

    if (m_ColorAttachments.empty() && !m_DepthAttachment)
    {
        for (auto& attachment : m_Specification.AliveAttachments)
        {
            GNT_ASSERT(m_Specification.Attachments.empty(), "You got attachments to create and you want to use a new one?");
            Ref<VulkanImage> vulkanImage = std::static_pointer_cast<VulkanImage>(attachment);

            if (!m_DepthAttachment && ImageUtils::IsDepthFormat(vulkanImage->GetSpecification().Format))
            {
                m_DepthAttachment = vulkanImage;
                continue;
            }

            m_ColorAttachments.push_back(vulkanImage);
        }
    }

    if (m_RenderPass)
    {
        for (auto& Framebuffer : m_Framebuffers)
            vkDestroyFramebuffer(logicalDevice, Framebuffer, nullptr);

        if (m_Specification.AliveAttachments.empty())
        {
            if (!m_ColorAttachments.empty())
            {
                for (uint32_t i = 0; i < swapchain->GetImageCount(); ++i)
                {
                    m_ColorAttachments[i]->GetSpecification().Width  = m_Specification.Width;
                    m_ColorAttachments[i]->GetSpecification().Height = m_Specification.Height;
                    m_ColorAttachments[i]->Invalidate();
                }
            }

            if (m_DepthAttachment)
            {
                m_DepthAttachment->GetSpecification().Width  = m_Specification.Width;
                m_DepthAttachment->GetSpecification().Height = m_Specification.Height;
                m_DepthAttachment->Invalidate();
            }
        }
    }

    // RenderPass creation
    if (!m_RenderPass)
    {
        std::vector<VkAttachmentDescription> attachments;
        // Color Attachment
        if (!m_ColorAttachments.empty())
        {
            VkAttachmentDescription colorAttachment = {};
            colorAttachment.format                  = m_ColorAttachments[0]->GetFormat();
            colorAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
            colorAttachment.loadOp                  = GauntletLoadOpToVulkan(framebufferColorAttachment.LoadOp);
            colorAttachment.storeOp                 = GauntletStoreOpToVulkan(framebufferColorAttachment.StoreOp);
            colorAttachment.initialLayout =
                framebufferColorAttachment.LoadOp == ELoadOp::LOAD ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments.push_back(colorAttachment);
        }

        // Depth Attachment
        if (m_DepthAttachment)
        {
            VkAttachmentDescription depthAttachment = {};
            depthAttachment.format                  = m_DepthAttachment->GetFormat();
            depthAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
            depthAttachment.loadOp                  = GauntletLoadOpToVulkan(framebufferDepthAttachment.LoadOp);
            depthAttachment.storeOp                 = GauntletStoreOpToVulkan(framebufferDepthAttachment.StoreOp);

            depthAttachment.initialLayout =
                framebufferDepthAttachment.LoadOp == ELoadOp::LOAD ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

            attachments.emplace_back(depthAttachment);
        }

        VkSubpassDescription subpassDesc = {};
        subpassDesc.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;

        uint32_t currentAttachment = 0;
        std::vector<VkAttachmentReference> attachmentRefs;
        if (!m_ColorAttachments.empty())
        {
            attachmentRefs.emplace_back(currentAttachment, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            subpassDesc.colorAttachmentCount = 1;
            subpassDesc.pColorAttachments    = &attachmentRefs[currentAttachment];
            ++currentAttachment;
        }

        if (m_DepthAttachment)
        {
            attachmentRefs.emplace_back(currentAttachment, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            subpassDesc.pDepthStencilAttachment = &attachmentRefs[currentAttachment];

            // After vector emplace_back everything is messed up, this way we fix it
            if (!m_ColorAttachments.empty()) subpassDesc.pColorAttachments = &attachmentRefs[currentAttachment - 1];
            ++currentAttachment;
        }

        /* Some explanation about subpass dependency
         * srcSubpass:
         * Index of subpass we depend on(current subpass won't start until srcSubpass has finished execution)
         * If we wanted to depend on a subpass that's part of a previous RenderPass, we could just pass in VK_SUBPASS_EXTERNAL here
         * instead. In this case, that would mean "wait for all of the subpasses within all of the render passes before this one".
         *
         * dstSubpass: Index to the current subpass, i.e. the one this dependency exists for. (Current subpass that starts)
         *
         * srcStageMask: Finish executing srcSubpass on this stage before we move onto current(dstSubpass).
         * dstStageMask: Is a bitmask of all of the Vulkan stages in dstSubpass that we're not allowed to execute until after the stages
         * in srcStageMask have completed within srcSubpass. IMO. This is the stage that your dstSubpass gonna execute only when
         * srcSubpass completed his srcStageMask.
         *
         * Okay, so, now we've specified the execution dependencies (the order in which these subpasses must execute) between our two
         * subpasses. But GPUs are complicated beasts that do a lot of caching of images and such, so just specifying the order we need
         * these rendering commands to occur in actually isn't enough. We also need to tell Vulkan the memory access types we need and
         * when we need them, so it can update caches and such accordingly.
         *
         * srcAccessMask: Is a bitmask of all the Vulkan memory access types used by srcSubpass
         * dstAccessMask: Is a bitmask of all the Vulkan memory access types we're going to use in dstSubpass.
         *
         * Think of it like we're saying: "after you've finished writing to the color attachment in srcSubpass, 'flush' the results as
         * needed for us to be able to read it in our shaders."
         *
         */
        std::vector<VkSubpassDependency> dependencies;

        if (!m_ColorAttachments.empty())
        {
            // Color dependency

            VkSubpassDependency colorDependency = {};
            colorDependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
            colorDependency.dstSubpass          = 0;
            colorDependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            colorDependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            colorDependency.srcAccessMask       = 0;
            colorDependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies.push_back(colorDependency);
        }

        // Depth dependency
        if (m_DepthAttachment)
        {
            VkSubpassDependency depthDependency = {};
            depthDependency.srcSubpass          = 0;
            depthDependency.dstSubpass          = VK_SUBPASS_EXTERNAL;
            depthDependency.srcStageMask        = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            depthDependency.dstStageMask        = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            depthDependency.srcAccessMask       = 0;
            depthDependency.dstAccessMask       = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            dependencies.push_back(depthDependency);
        }

        VkRenderPassCreateInfo renderPassCreateInfo = {};
        renderPassCreateInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.attachmentCount        = static_cast<uint32_t>(attachments.size());
        renderPassCreateInfo.pAttachments           = attachments.data();
        renderPassCreateInfo.dependencyCount        = static_cast<uint32_t>(dependencies.size());
        renderPassCreateInfo.pDependencies          = dependencies.data();
        renderPassCreateInfo.subpassCount           = 1;
        renderPassCreateInfo.pSubpasses             = &subpassDesc;

        VK_CHECK(vkCreateRenderPass(logicalDevice, &renderPassCreateInfo, nullptr, &m_RenderPass), "Failed to create render pass!");
    }

    // Framebuffers creation
    std::vector<VkImageView> attachments;
    if (!m_ColorAttachments.empty()) attachments.push_back(m_ColorAttachments[0]->GetView());
    if (m_DepthAttachment) attachments.push_back(m_DepthAttachment->GetView());

    VkFramebufferCreateInfo framebufferCreateInfo = {};
    framebufferCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.renderPass              = m_RenderPass;
    framebufferCreateInfo.layers                  = 1;
    framebufferCreateInfo.pAttachments            = attachments.data();

    framebufferCreateInfo.attachmentCount = attachments.size();
    framebufferCreateInfo.width           = m_Specification.Width;
    framebufferCreateInfo.height          = m_Specification.Height;

    m_Framebuffers.resize(swapchain->GetImageCount());
    for (uint32_t i = 0; i < m_Framebuffers.size(); ++i)
    {
        if (!m_ColorAttachments.empty()) attachments[0] = m_ColorAttachments[i]->GetView();
        VK_CHECK(vkCreateFramebuffer(logicalDevice, &framebufferCreateInfo, nullptr, &m_Framebuffers[i]), "Failed to create framebuffer!");
    }

    if (!m_ColorAttachments.empty() && m_ClearValues.size() < 2)
    {
        VkClearColorValue clearColor = {framebufferColorAttachment.ClearColor.r, framebufferColorAttachment.ClearColor.g,
                                        framebufferColorAttachment.ClearColor.b, framebufferColorAttachment.ClearColor.a};
        VkClearValue clearValue      = {};
        clearValue.color             = clearColor;
        m_ClearValues.push_back(clearValue);
    }

    if (m_DepthAttachment && m_ClearValues.size() < 2)
    {
        VkClearDepthStencilValue depthStencil = {framebufferDepthAttachment.ClearColor.x, 0};

        VkClearValue clearValue{};
        clearValue.depthStencil = depthStencil;

        m_ClearValues.push_back(clearValue);
    }
}

void VulkanFramebuffer::BeginRenderPass(const VkCommandBuffer& commandBuffer)
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(Context.GetSwapchain()->IsValid(), "Vulkan swapchain is not valid!");

    VkRenderPassBeginInfo RenderPassBeginInfo = {};
    RenderPassBeginInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RenderPassBeginInfo.framebuffer           = m_Framebuffers[Context.GetSwapchain()->GetCurrentImageIndex()];
    RenderPassBeginInfo.pClearValues          = m_ClearValues.data();
    RenderPassBeginInfo.clearValueCount       = m_ClearValues.size();
    RenderPassBeginInfo.renderPass            = m_RenderPass;

    VkRect2D RenderArea            = {};
    RenderArea.offset              = {0, 0};
    RenderArea.extent              = VkExtent2D{m_Specification.Width, m_Specification.Height};
    RenderPassBeginInfo.renderArea = RenderArea;

    vkCmdBeginRenderPass(commandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanFramebuffer::Destroy()
{
    if (m_Specification.AliveAttachments.empty())
    {
        for (auto& colorAttachment : m_ColorAttachments)
            colorAttachment->Destroy();

        if (m_DepthAttachment) m_DepthAttachment->Destroy();
    }

    auto& context           = (VulkanContext&)VulkanContext::Get();
    VkDevice& logicalDevice = context.GetDevice()->GetLogicalDevice();

    vkDestroyRenderPass(logicalDevice, m_RenderPass, nullptr);

    for (auto& framebuffer : m_Framebuffers)
        vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
}

const uint32_t VulkanFramebuffer::GetWidth() const
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetSwapchain()->IsValid(), "Vulkan swapchain is not valid!");

    if (m_ColorAttachments.size() == context.GetSwapchain()->GetImageCount())
    {
        return m_ColorAttachments[context.GetSwapchain()->GetCurrentImageIndex()]->GetWidth();
    }

    if (m_DepthAttachment) return m_DepthAttachment->GetWidth();

    return m_ColorAttachments[0]->GetWidth();
}

const uint32_t VulkanFramebuffer::GetHeight() const
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetSwapchain()->IsValid(), "Vulkan swapchain is not valid!");

    if (m_ColorAttachments.size() == context.GetSwapchain()->GetImageCount())
    {
        return m_ColorAttachments[context.GetSwapchain()->GetCurrentImageIndex()]->GetHeight();
    }

    if (m_DepthAttachment) return m_DepthAttachment->GetHeight();

    return m_ColorAttachments[0]->GetHeight();
}

}  // namespace Gauntlet
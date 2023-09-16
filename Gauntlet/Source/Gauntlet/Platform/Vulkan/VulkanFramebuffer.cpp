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
        case ELoadOp::DONT_CARE: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        case ELoadOp::CLEAR: return VK_ATTACHMENT_LOAD_OP_CLEAR;
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

    Create();
}

void VulkanFramebuffer::Create()
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetSwapchain()->IsValid(), "Vulkan swapchain is not valid!");

    if (!m_Specification.ExistingAttachments.empty())
        GNT_ASSERT(m_Specification.Attachments.empty(), "You got existing attachments and you want to create new?");

    if (!m_Specification.Attachments.empty())
        GNT_ASSERT(m_Specification.ExistingAttachments.empty(), "You want to create attachments and you specified existing?");

    bool bHaveDepthAttachment = false;
    bool bHaveColorAttachment = false;
    for (auto& attachment : m_Specification.Attachments)
    {
        if (!bHaveDepthAttachment) bHaveDepthAttachment = ImageUtils::IsDepthFormat(attachment.Format);
        if (!bHaveColorAttachment) bHaveColorAttachment = !ImageUtils::IsDepthFormat(attachment.Format);

        FramebufferAttachment newAttachment = {};
        ImageSpecification imageSpec        = {};
        imageSpec.Width                     = m_Specification.Width;
        imageSpec.Height                    = m_Specification.Height;
        imageSpec.Format                    = attachment.Format;
        imageSpec.Usage                     = EImageUsage::Attachment;
        imageSpec.CreateTextureID           = true;
        imageSpec.Filter                    = attachment.Filter;
        imageSpec.Wrap                      = attachment.Wrap;

        for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
        {
            newAttachment.Attachments[i] = MakeRef<VulkanImage>(imageSpec);
        }

        newAttachment.Specification.Format     = attachment.Format;
        newAttachment.Specification.Filter     = attachment.Filter;
        newAttachment.Specification.Wrap       = attachment.Wrap;
        newAttachment.Specification.ClearColor = attachment.ClearColor;
        newAttachment.Specification.LoadOp     = attachment.LoadOp;
        newAttachment.Specification.StoreOp    = attachment.StoreOp;

        m_Attachments.push_back(newAttachment);
    }

    for (uint32_t i = 0; i < m_Specification.ExistingAttachments.size(); ++i)
    {
        FramebufferAttachment framebufferAttachment = {};

        for (uint32_t k = 0; k < FRAMES_IN_FLIGHT; ++k)
        {
            framebufferAttachment.Specification.Format = m_Specification.ExistingAttachments[i].Attachments[k]->GetSpecification().Format;
            framebufferAttachment.Attachments[k]       = m_Specification.ExistingAttachments[i].Attachments[k];
        }
        m_Attachments.push_back(framebufferAttachment);
    }

    // RenderPass creation
    VkSubpassDescription subpassDesc = {};
    subpassDesc.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;

    std::vector<VkAttachmentDescription> attachments(m_Attachments.size());
    std::vector<VkAttachmentReference> attachmentRefs(m_Attachments.size());

    uint32_t currentAttachment = 0;
    for (auto& attachment : m_Attachments)
    {
        VkAttachmentDescription attachmentDescription = {};
        attachmentDescription.format                  = ImageUtils::GauntletImageFormatToVulkan(attachment.Specification.Format);
        attachmentDescription.samples                 = VK_SAMPLE_COUNT_1_BIT;
        attachmentDescription.loadOp                  = GauntletLoadOpToVulkan(attachment.Specification.LoadOp);
        attachmentDescription.storeOp                 = GauntletStoreOpToVulkan(attachment.Specification.StoreOp);
        attachmentDescription.initialLayout =
            attachment.Specification.LoadOp == ELoadOp::LOAD ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentDescription.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        attachmentDescription.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        attachments[currentAttachment] = attachmentDescription;

        const bool bIsDepthAttachment = ImageUtils::IsDepthFormat(attachment.Specification.Format);

        attachmentRefs[currentAttachment].attachment = currentAttachment;
        attachmentRefs[currentAttachment].layout =
            bIsDepthAttachment ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        ++currentAttachment;
    }

    if (bHaveColorAttachment)
    {
        subpassDesc.colorAttachmentCount = static_cast<uint32_t>(m_Attachments.size()) - (uint32_t)bHaveDepthAttachment;
        subpassDesc.pColorAttachments    = &attachmentRefs[0];
    }

    if (bHaveDepthAttachment)
    {
        subpassDesc.pDepthStencilAttachment = &attachmentRefs[m_Attachments.size() - 1];
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
    if (bHaveColorAttachment)
    {
        VkSubpassDependency colorDependency = {};
        colorDependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
        colorDependency.dstSubpass          = 0;
        colorDependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        colorDependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        colorDependency.srcAccessMask       = 0;
        colorDependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies.push_back(colorDependency);
    }

    if (bHaveDepthAttachment)
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

    VK_CHECK(vkCreateRenderPass(context.GetDevice()->GetLogicalDevice(), &renderPassCreateInfo, nullptr, &m_RenderPass),
             "Failed to create render pass!");

    // Framebuffers creation
    VkFramebufferCreateInfo framebufferCreateInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    framebufferCreateInfo.renderPass              = m_RenderPass;
    framebufferCreateInfo.layers                  = 1;

    framebufferCreateInfo.width  = m_Specification.Width;
    framebufferCreateInfo.height = m_Specification.Height;

    m_Framebuffers.resize(FRAMES_IN_FLIGHT);
    for (uint32_t i = 0; i < m_Framebuffers.size(); ++i)
    {
        std::vector<VkImageView> imageViews;
        for (uint32_t k = 0; k < m_Attachments.size(); ++k)
        {
            Ref<VulkanImage> vulkanImage = static_pointer_cast<VulkanImage>(m_Attachments[k].Attachments[i]);
            imageViews.push_back(vulkanImage->GetView());
        }
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(imageViews.size());
        framebufferCreateInfo.pAttachments    = imageViews.data();

        VK_CHECK(vkCreateFramebuffer(context.GetDevice()->GetLogicalDevice(), &framebufferCreateInfo, nullptr, &m_Framebuffers[i]),
                 "Failed to create framebuffer!");
    }

    // Filling clear colors;
    for (auto& framebufferAttachment : m_Attachments)
    {
        VkClearValue clearValue = {};

        if (ImageUtils::IsDepthFormat(framebufferAttachment.Specification.Format))
        {
            clearValue.depthStencil = {framebufferAttachment.Specification.ClearColor.x, 0};
        }
        else
        {
            clearValue.color = {framebufferAttachment.Specification.ClearColor.r, framebufferAttachment.Specification.ClearColor.g,
                                framebufferAttachment.Specification.ClearColor.b, framebufferAttachment.Specification.ClearColor.a};
        }

        m_ClearValues.push_back(clearValue);
    }
}

void VulkanFramebuffer::Invalidate()
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetSwapchain()->IsValid(), "Vulkan swapchain is not valid!");

    const auto& swapchain     = context.GetSwapchain();
    const auto& logicalDevice = context.GetDevice()->GetLogicalDevice();

    /*newAttachment.Attachments[i]->GetSpecification().Width  = m_Specification.Width;
    newAttachment.Attachments[i]->GetSpecification().Height = m_Specification.Height;
    newAttachment.Attachments[i]->Invalidate();*/

    // Destroy();
    // Create();

    vkDestroyRenderPass(logicalDevice, m_RenderPass, nullptr);

    for (auto& framebuffer : m_Framebuffers)
        vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);

    bool bHaveDepthAttachment = false;
    bool bHaveColorAttachment = false;

    for (auto& attachment : m_Attachments)
    {
        if (!bHaveDepthAttachment) bHaveDepthAttachment = ImageUtils::IsDepthFormat(attachment.Specification.Format);
        if (!bHaveColorAttachment) bHaveColorAttachment = !ImageUtils::IsDepthFormat(attachment.Specification.Format);

        // In case we own images. So we didn't specify any existing attachments.
        if (m_Specification.ExistingAttachments.empty())
        {
            for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
            {
                attachment.Attachments[i]->GetSpecification().Width  = m_Specification.Width;
                attachment.Attachments[i]->GetSpecification().Height = m_Specification.Height;
                attachment.Attachments[i]->Invalidate();
            }
        }
    }

    // RenderPass creation
    VkSubpassDescription subpassDesc = {};
    subpassDesc.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;

    std::vector<VkAttachmentDescription> attachments(m_Attachments.size());
    std::vector<VkAttachmentReference> attachmentRefs(m_Attachments.size());

    uint32_t currentAttachment = 0;
    for (auto& attachment : m_Attachments)
    {
        VkAttachmentDescription attachmentDescription = {};
        attachmentDescription.format                  = ImageUtils::GauntletImageFormatToVulkan(attachment.Specification.Format);
        attachmentDescription.samples                 = VK_SAMPLE_COUNT_1_BIT;
        attachmentDescription.loadOp                  = GauntletLoadOpToVulkan(attachment.Specification.LoadOp);
        attachmentDescription.storeOp                 = GauntletStoreOpToVulkan(attachment.Specification.StoreOp);
        attachmentDescription.initialLayout =
            attachment.Specification.LoadOp == ELoadOp::LOAD ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentDescription.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        attachmentDescription.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        attachments[currentAttachment] = attachmentDescription;

        const bool bIsDepthAttachment = ImageUtils::IsDepthFormat(attachment.Specification.Format);

        attachmentRefs[currentAttachment].attachment = currentAttachment;
        attachmentRefs[currentAttachment].layout =
            bIsDepthAttachment ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        ++currentAttachment;
    }

    if (bHaveColorAttachment)
    {
        subpassDesc.colorAttachmentCount = static_cast<uint32_t>(m_Attachments.size()) - (uint32_t)bHaveDepthAttachment;
        subpassDesc.pColorAttachments    = &attachmentRefs[0];
    }

    if (bHaveDepthAttachment)
    {
        subpassDesc.pDepthStencilAttachment = &attachmentRefs[m_Attachments.size() - 1];
    }

    std::vector<VkSubpassDependency> dependencies;

    if (bHaveColorAttachment)
    {
        VkSubpassDependency colorDependency = {};
        colorDependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
        colorDependency.dstSubpass          = 0;
        colorDependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        colorDependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        colorDependency.srcAccessMask       = 0;
        colorDependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies.push_back(colorDependency);
    }

    if (bHaveDepthAttachment)
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

    // Framebuffers creation
    VkFramebufferCreateInfo framebufferCreateInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    framebufferCreateInfo.renderPass              = m_RenderPass;
    framebufferCreateInfo.layers                  = 1;

    framebufferCreateInfo.width  = m_Specification.Width;
    framebufferCreateInfo.height = m_Specification.Height;

    m_Framebuffers.resize(FRAMES_IN_FLIGHT);
    for (uint32_t i = 0; i < m_Framebuffers.size(); ++i)
    {
        std::vector<VkImageView> imageViews;
        for (uint32_t k = 0; k < m_Attachments.size(); ++k)
        {
            Ref<VulkanImage> vulkanImage = static_pointer_cast<VulkanImage>(m_Attachments[k].Attachments[i]);
            imageViews.push_back(vulkanImage->GetView());
        }
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(imageViews.size());
        framebufferCreateInfo.pAttachments    = imageViews.data();

        VK_CHECK(vkCreateFramebuffer(logicalDevice, &framebufferCreateInfo, nullptr, &m_Framebuffers[i]), "Failed to create framebuffer!");
    }
}

void VulkanFramebuffer::BeginRenderPass(const VkCommandBuffer& commandBuffer, const VkSubpassContents subpassContents)
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetSwapchain()->IsValid(), "Vulkan swapchain is not valid!");

    VkRenderPassBeginInfo RenderPassBeginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    RenderPassBeginInfo.framebuffer           = m_Framebuffers[context.GetSwapchain()->GetCurrentFrameIndex()];
    RenderPassBeginInfo.clearValueCount       = static_cast<uint32_t>(m_ClearValues.size());
    RenderPassBeginInfo.pClearValues          = m_ClearValues.data();
    RenderPassBeginInfo.renderPass            = m_RenderPass;

    VkRect2D RenderArea            = {};
    RenderArea.offset              = {0, 0};
    RenderArea.extent              = VkExtent2D{m_Specification.Width, m_Specification.Height};
    RenderPassBeginInfo.renderArea = RenderArea;

    vkCmdBeginRenderPass(commandBuffer, &RenderPassBeginInfo, subpassContents);
}

void VulkanFramebuffer::Destroy()
{
    if (m_Specification.ExistingAttachments.empty())
    {
        for (auto& attachment : m_Attachments)
        {
            for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
                attachment.Attachments[i]->Destroy();
        }
    }

    auto& context           = (VulkanContext&)VulkanContext::Get();
    VkDevice& logicalDevice = context.GetDevice()->GetLogicalDevice();

    vkDestroyRenderPass(logicalDevice, m_RenderPass, nullptr);

    for (auto& framebuffer : m_Framebuffers)
        vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
}

const Ref<VulkanImage> VulkanFramebuffer::GetDepthAttachment() const
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetSwapchain()->IsValid(), "Swapchain is not valid!");

    for (auto& framebufferAttachment : m_Attachments)
    {
        if (ImageUtils::IsDepthFormat(framebufferAttachment.Specification.Format))
            return static_pointer_cast<VulkanImage>(framebufferAttachment.Attachments[context.GetSwapchain()->GetCurrentFrameIndex()]);
    }

    return nullptr;
}

const VkFramebuffer& VulkanFramebuffer::Get() const
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetSwapchain()->IsValid(), "Swapchain is not valid!");

    return m_Framebuffers[context.GetSwapchain()->GetCurrentFrameIndex()];
}

void VulkanFramebuffer::SetDepthStencilClearColor(const float depth, const uint32_t stencil)
{
    for (uint32_t i = 0; i < m_Attachments.size(); ++i)
    {
        if (ImageUtils::IsDepthFormat(m_Attachments[i].Specification.Format))
        {
            m_ClearValues[i].depthStencil = {depth, stencil};
            break;
        }
    }
}

}  // namespace Gauntlet
#include "GauntletPCH.h"
#include "VulkanRenderPass.h"

#include "VulkanContext.h"

namespace Gauntlet
{
VulkanRenderPass::VulkanRenderPass(const RenderPassSpecification& renderPassSpecification) {}

VulkanRenderPass::~VulkanRenderPass()
{
    Destroy();
}

void VulkanRenderPass::Begin(const VkCommandBuffer& commandBuffer, const VkSubpassContents subpassContents)
{
    VkRenderPassBeginInfo beginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    beginInfo.renderPass            = m_Handle;
    beginInfo.renderArea            = VkRect2D({0, 0}, {800, 600});

    vkCmdBeginRenderPass(commandBuffer, &beginInfo, subpassContents);
}

void VulkanRenderPass::Invalidate()
{
    if (m_Handle) Destroy();

    VkSubpassDescription subpassDesc = {};
    subpassDesc.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;

    // std::vector<VkAttachmentDescription> attachments(m_Attachments.size());
    // std::vector<VkAttachmentReference> attachmentRefs(m_Attachments.size());

    // uint32_t currentAttachment = 0;
    // for (auto& attachment : m_Attachments)
    //{
    //     VkAttachmentDescription attachmentDescription = {};
    //     attachmentDescription.format                  = ImageUtils::GauntletImageFormatToVulkan(attachment.Specification.Format);
    //     attachmentDescription.samples                 = VK_SAMPLE_COUNT_1_BIT;
    //     attachmentDescription.loadOp                  = GauntletLoadOpToVulkan(attachment.Specification.LoadOp);
    //     attachmentDescription.storeOp                 = GauntletStoreOpToVulkan(attachment.Specification.StoreOp);
    //     attachmentDescription.initialLayout =
    //         attachment.Specification.LoadOp == ELoadOp::LOAD ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
    //     attachmentDescription.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    //     attachmentDescription.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    //     attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    //    attachments[currentAttachment] = attachmentDescription;

    //    const bool bIsDepthAttachment = ImageUtils::IsDepthFormat(attachment.Specification.Format);

    //    attachmentRefs[currentAttachment].attachment = currentAttachment;
    //    attachmentRefs[currentAttachment].layout =
    //        bIsDepthAttachment ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    //    ++currentAttachment;
    //}

    // if (bHaveColorAttachment)
    //{
    //     subpassDesc.colorAttachmentCount = static_cast<uint32_t>(m_Attachments.size()) - (uint32_t)bHaveDepthAttachment;
    //     subpassDesc.pColorAttachments    = &attachmentRefs[0];
    // }

    // if (bHaveDepthAttachment)
    //{
    //     subpassDesc.pDepthStencilAttachment = &attachmentRefs[m_Attachments.size() - 1];
    // }

    // std::vector<VkSubpassDependency> dependencies;
    // if (bHaveColorAttachment)
    //{
    //     VkSubpassDependency colorDependency = {};
    //     colorDependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
    //     colorDependency.dstSubpass          = 0;
    //     colorDependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //     colorDependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //     colorDependency.srcAccessMask       = 0;
    //     colorDependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    //     colorDependency.dependencyFlags     = VK_DEPENDENCY_BY_REGION_BIT;

    //    dependencies.push_back(colorDependency);
    //}

    // if (bHaveDepthAttachment)
    //{
    //     VkSubpassDependency depthDependency = {};
    //     depthDependency.srcSubpass          = 0;
    //     depthDependency.dstSubpass          = VK_SUBPASS_EXTERNAL;
    //     depthDependency.srcStageMask        = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    //     depthDependency.dstStageMask        = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    //     depthDependency.srcAccessMask       = 0;
    //     depthDependency.dstAccessMask       = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    //     depthDependency.dependencyFlags     = VK_DEPENDENCY_BY_REGION_BIT;

    //    dependencies.push_back(depthDependency);
    //}

    // VkRenderPassCreateInfo renderPassCreateInfo = {};
    // renderPassCreateInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    // renderPassCreateInfo.attachmentCount        = static_cast<uint32_t>(attachments.size());
    // renderPassCreateInfo.pAttachments           = attachments.data();
    // renderPassCreateInfo.dependencyCount        = static_cast<uint32_t>(dependencies.size());
    // renderPassCreateInfo.pDependencies          = dependencies.data();
    // renderPassCreateInfo.subpassCount           = 1;
    // renderPassCreateInfo.pSubpasses             = &subpassDesc;

    // VK_CHECK(vkCreateRenderPass(context.GetDevice()->GetLogicalDevice(), &renderPassCreateInfo, nullptr, &m_RenderPass),
    //          "Failed to create render pass!");
}

void VulkanRenderPass::Destroy() {}

}  // namespace Gauntlet
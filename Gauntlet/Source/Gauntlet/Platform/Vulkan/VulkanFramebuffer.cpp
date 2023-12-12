#include "GauntletPCH.h"
#include "VulkanFramebuffer.h"

#include "VulkanUtility.h"
#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanImage.h"
#include "VulkanDevice.h"
#include "VulkanCommandBuffer.h"

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

    Invalidate();
}

void VulkanFramebuffer::Invalidate()
{
    auto& context             = (VulkanContext&)VulkanContext::Get();
    const auto& logicalDevice = context.GetDevice()->GetLogicalDevice();
    context.GetDevice()->WaitDeviceOnFinish();

    if (!m_Specification.ExistingAttachments.empty())
        GNT_ASSERT(m_Specification.Attachments.empty(), "You got existing attachments and you want to create new?");

    if (!m_Specification.Attachments.empty())
        GNT_ASSERT(m_Specification.ExistingAttachments.empty(), "You want to create attachments and you specified existing?");

    // Don't destroy what you don't own
    if (m_Specification.ExistingAttachments.empty())
    {
        for (auto& fbAttachment : m_Attachments)
        {
            fbAttachment.Attachment->GetSpecification().Width  = m_Specification.Width;
            fbAttachment.Attachment->GetSpecification().Height = m_Specification.Height;
            fbAttachment.Attachment->Invalidate();
        }
    }

    m_AttachmentInfos.clear();

    // In case we wanna be owners
    if (m_Attachments.empty() && !m_Specification.Attachments.empty())
    {
        FramebufferAttachment newfbAttachment = {};
        ImageSpecification imageSpec          = {};

        for (auto& fbAttchament : m_Specification.Attachments)
        {
            imageSpec.Format          = fbAttchament.Format;
            imageSpec.Filter          = fbAttchament.Filter;
            imageSpec.Wrap            = fbAttchament.Wrap;
            imageSpec.CreateTextureID = true;
            imageSpec.Height          = m_Specification.Height;
            imageSpec.Width           = m_Specification.Width;
            imageSpec.Usage           = EImageUsage::Attachment;

            newfbAttachment.Attachment    = Image::Create(imageSpec);
            newfbAttachment.Specification = fbAttchament;  // copy the whole framebuffer attachment specification

            m_Attachments.push_back(newfbAttachment);
        }
    }

    m_DepthStencilAttachmentInfo                  = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    VkRenderingAttachmentInfo depthAttachmentInfo = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    // TODO: Revisit with stenciling.

    // In case we got existing
    for (auto& fbAttachment : m_Specification.ExistingAttachments)
    {
        auto vulkanImage = static_pointer_cast<VulkanImage>(fbAttachment.Attachment);
        GNT_ASSERT(vulkanImage, "Failed to cast Image to VulkanImage!");

        if (ImageUtils::IsDepthFormat(fbAttachment.Specification.Format))
        {
            depthAttachmentInfo.imageView               = vulkanImage->GetView();
            depthAttachmentInfo.storeOp                 = GauntletStoreOpToVulkan(m_Specification.StoreOp);
            depthAttachmentInfo.loadOp                  = GauntletLoadOpToVulkan(m_Specification.LoadOp);
            depthAttachmentInfo.imageLayout             = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            depthAttachmentInfo.clearValue.depthStencil = {1.0f, 0 /*128*/};
        }
        else
        {
            auto& attachment            = m_AttachmentInfos.emplace_back(VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO);
            attachment.imageView        = vulkanImage->GetView();
            attachment.storeOp          = GauntletStoreOpToVulkan(m_Specification.StoreOp);
            attachment.loadOp           = GauntletLoadOpToVulkan(m_Specification.LoadOp);
            attachment.imageLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachment.clearValue.color = {fbAttachment.Specification.ClearColor.x, fbAttachment.Specification.ClearColor.y,
                                           fbAttachment.Specification.ClearColor.z, fbAttachment.Specification.ClearColor.w};
        }
    }

    // Otherwise check for these
    for (auto& fbAttachment : m_Attachments)
    {
        auto vulkanImage = static_pointer_cast<VulkanImage>(fbAttachment.Attachment);
        GNT_ASSERT(vulkanImage, "Failed to cast Image to VulkanImage!");

        if (ImageUtils::IsDepthFormat(fbAttachment.Specification.Format))
        {
            depthAttachmentInfo.imageView               = vulkanImage->GetView();
            depthAttachmentInfo.storeOp                 = GauntletStoreOpToVulkan(fbAttachment.Specification.StoreOp);
            depthAttachmentInfo.loadOp                  = GauntletLoadOpToVulkan(fbAttachment.Specification.LoadOp);
            depthAttachmentInfo.imageLayout             = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            depthAttachmentInfo.clearValue.depthStencil = {1.0f, 0 /*128*/};
        }
        else
        {
            auto& attachment            = m_AttachmentInfos.emplace_back(VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO);
            attachment.imageView        = vulkanImage->GetView();
            attachment.storeOp          = GauntletStoreOpToVulkan(fbAttachment.Specification.StoreOp);
            attachment.loadOp           = GauntletLoadOpToVulkan(fbAttachment.Specification.LoadOp);
            attachment.imageLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachment.clearValue.color = {fbAttachment.Specification.ClearColor.x, fbAttachment.Specification.ClearColor.y,
                                           fbAttachment.Specification.ClearColor.z, fbAttachment.Specification.ClearColor.w};
        }
    }

    // Easy to handle them if depth is the last attachment
    if (depthAttachmentInfo.imageView) m_AttachmentInfos.push_back(depthAttachmentInfo);
}

void VulkanFramebuffer::Destroy()
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    context.GetDevice()->WaitDeviceOnFinish();

    // Don't destroy what you don't own
    if (m_Specification.ExistingAttachments.empty())
    {
        for (auto& fbAttachment : m_Attachments)
            fbAttachment.Attachment->Destroy();
    }

    m_Attachments.clear();
    m_AttachmentInfos.clear();
}

void VulkanFramebuffer::BeginPass(const Ref<CommandBuffer>& commandBuffer)
{
    auto vulkanCommandBuffer = static_pointer_cast<VulkanCommandBuffer>(commandBuffer);
    GNT_ASSERT(vulkanCommandBuffer, "Failed to cast CommandBuffer to VulkanCommandBuffer");

    vulkanCommandBuffer->BeginDebugLabel(m_Specification.Name.data(), glm::vec4(1.0f));

    // In case we own them
    // TODO: REVISIT THIS SHIT

    VkRenderingAttachmentInfo depthAttachmentInfo = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    for (uint32_t i = 0; i < m_AttachmentInfos.size(); ++i)
    {
        auto vulkanImage = static_pointer_cast<VulkanImage>(m_Attachments.size() > 0 ? m_Attachments[i].Attachment
                                                                                     : m_Specification.ExistingAttachments[i].Attachment);
        GNT_ASSERT(vulkanImage, "Failed to cast Imaage to VulkanImage");

        VkImageMemoryBarrier imageBarrier            = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        imageBarrier.image                           = vulkanImage->Get();
        imageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.subresourceRange.baseArrayLayer = 0;
        imageBarrier.subresourceRange.baseMipLevel   = 0;

        imageBarrier.subresourceRange.levelCount = vulkanImage->GetSpecification().Mips;
        imageBarrier.subresourceRange.layerCount = vulkanImage->GetSpecification().Layers;

        imageBarrier.oldLayout = vulkanImage->GetLayout();

        imageBarrier.srcAccessMask = 0;
        if (ImageUtils::IsDepthFormat(vulkanImage->GetSpecification().Format))
        {
            depthAttachmentInfo    = m_AttachmentInfos[i];
            imageBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            vulkanImage->SetLayout(imageBarrier.newLayout);

            imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            imageBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            vulkanCommandBuffer->InsertBarrier(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                               VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                               VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &imageBarrier);
        }
        else
        {
            imageBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            vulkanImage->SetLayout(imageBarrier.newLayout);

            imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

            imageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            vulkanCommandBuffer->InsertBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                               VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &imageBarrier);
        }
    }

    VkRenderingInfo renderingInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO};
    if (vulkanCommandBuffer->GetLevel() == ECommandBufferLevel::COMMAND_BUFFER_LEVEL_SECONDARY)
        renderingInfo.flags |= VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT;

    renderingInfo.renderArea           = {0, 0, m_Specification.Width, m_Specification.Height};
    renderingInfo.layerCount           = 1;
    renderingInfo.colorAttachmentCount = static_cast<uint32_t>(m_AttachmentInfos.size()) - (depthAttachmentInfo.imageView ? 1 : 0);
    renderingInfo.pColorAttachments    = m_AttachmentInfos.data();
    if (depthAttachmentInfo.imageView) renderingInfo.pDepthAttachment = &depthAttachmentInfo;
    // renderingInfo.pStencilAttachment   = &m_DepthStencilAttachmentInfo;

    vkCmdBeginRendering(vulkanCommandBuffer->Get(), &renderingInfo);
}

void VulkanFramebuffer::EndPass(const Ref<CommandBuffer>& commandBuffer)
{
    auto vulkanCommandBuffer = static_pointer_cast<VulkanCommandBuffer>(commandBuffer);
    GNT_ASSERT(vulkanCommandBuffer, "Failed to cast CommandBuffer to VulkanCommandBuffer");

    vkCmdEndRendering(vulkanCommandBuffer->Get());

    vulkanCommandBuffer->EndDebugLabel();

    for (uint32_t i = 0; i < m_AttachmentInfos.size(); ++i)
    {
        auto vulkanImage = static_pointer_cast<VulkanImage>(m_Attachments.size() > 0 ? m_Attachments[i].Attachment
                                                                                     : m_Specification.ExistingAttachments[i].Attachment);

        VkImageMemoryBarrier imageBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        imageBarrier.image                = vulkanImage->Get();
        imageBarrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;

        imageBarrier.oldLayout = vulkanImage->GetLayout();
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        vulkanImage->SetLayout(imageBarrier.newLayout);

        imageBarrier.subresourceRange.baseArrayLayer = 0;
        imageBarrier.subresourceRange.baseMipLevel   = 0;
        imageBarrier.subresourceRange.levelCount     = vulkanImage->GetSpecification().Mips;
        imageBarrier.subresourceRange.layerCount     = vulkanImage->GetSpecification().Layers;
        imageBarrier.dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
        if (ImageUtils::IsDepthFormat(vulkanImage->GetSpecification().Format))
        {
            imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            imageBarrier.srcAccessMask               = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            vulkanCommandBuffer->InsertBarrier(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr,
                                               1, &imageBarrier);
        }
        else
        {
            imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarrier.srcAccessMask               = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            vulkanCommandBuffer->InsertBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                               VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &imageBarrier);
        }
    }
}

const Ref<VulkanImage> VulkanFramebuffer::GetDepthAttachment() const
{
    // In case we own attachments
    for (auto& framebufferAttachment : m_Attachments)
    {
        if (ImageUtils::IsDepthFormat(framebufferAttachment.Specification.Format))
            return static_pointer_cast<VulkanImage>(framebufferAttachment.Attachment);
    }

    for (auto& framebufferAttachment : m_Specification.ExistingAttachments)
    {
        if (ImageUtils::IsDepthFormat(framebufferAttachment.Specification.Format))
            return static_pointer_cast<VulkanImage>(framebufferAttachment.Attachment);
    }

    return nullptr;
}

}  // namespace Gauntlet
#include "EclipsePCH.h"
#include "VulkanSwapchain.h"

#include "VulkanCore.h"
#include "VulkanUtility.h"
#include "VulkanDevice.h"
#include "VulkanContext.h"
#include "VulkanImage.h"

#include "Eclipse/Core/Application.h"
#include "Eclipse/Core/Window.h"

#include "SwapchainSupportDetails.h"

namespace Eclipse
{
VulkanSwapchain::VulkanSwapchain(Scoped<VulkanDevice>& InDevice, VkSurfaceKHR& InSurface)
    : m_Device(InDevice), m_Surface(InSurface), m_DepthImage(nullptr)
{
    Invalidate();
}

void VulkanSwapchain::BeginRenderPass(const VkCommandBuffer& InCommandBuffer)
{
    std::vector<VkClearValue> ClearValues(2);
    ClearValues[0].color        = {m_ClearColor};
    ClearValues[1].depthStencil = {1.0f};

    VkRenderPassBeginInfo RenderPassBeginInfo = {};
    RenderPassBeginInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RenderPassBeginInfo.framebuffer           = m_Framebuffers[m_ImageIndex];
    RenderPassBeginInfo.pClearValues          = ClearValues.data();
    RenderPassBeginInfo.clearValueCount       = static_cast<uint32_t>(ClearValues.size());
    RenderPassBeginInfo.renderPass            = m_RenderPass;

    VkRect2D RenderArea            = {};
    RenderArea.offset              = {0, 0};
    RenderArea.extent              = m_SwapchainImageExtent;
    RenderPassBeginInfo.renderArea = RenderArea;

    vkCmdBeginRenderPass(InCommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

bool VulkanSwapchain::TryAcquireNextImage(const VkSemaphore& InImageAcquiredSemaphore, const VkFence& InFence)
{
    const auto result =
        vkAcquireNextImageKHR(m_Device->GetLogicalDevice(), m_Swapchain, UINT64_MAX, InImageAcquiredSemaphore, InFence, &m_ImageIndex);
    if (result == VK_SUCCESS) return true;

    if (result != VK_SUBOPTIMAL_KHR || result != VK_ERROR_OUT_OF_DATE_KHR)
    {
        ELS_ASSERT(false, "Failed to acquire image from the swapchain! Result is: %s", GetStringVulkanResult(result));
    }

    return false;
}

bool VulkanSwapchain::TryPresentImage(const VkSemaphore& InRenderFinishedSemaphore)
{
    VkPresentInfoKHR PresentInfo   = {};
    PresentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    PresentInfo.waitSemaphoreCount = 1;
    PresentInfo.pWaitSemaphores    = &InRenderFinishedSemaphore;
    PresentInfo.pImageIndices      = &m_ImageIndex;
    PresentInfo.swapchainCount     = 1;
    PresentInfo.pSwapchains        = &m_Swapchain;

    const auto result = vkQueuePresentKHR(m_Device->GetPresentQueue(), &PresentInfo);
    if (result == VK_SUCCESS)
    {
        m_FrameIndex = (m_FrameIndex + 1) % FRAMES_IN_FLIGHT;
        return true;
    }

    return false;
}

void VulkanSwapchain::Invalidate()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    if (m_DepthImage) m_DepthImage->Destroy();
    if (m_RenderPass) DestroyRenderPass();

    const auto OldSwapchain = m_Swapchain;
    if (OldSwapchain)
    {
        for (auto& ImageView : m_SwapchainImageViews)
        {
            vkDestroyImageView(m_Device->GetLogicalDevice(), ImageView, nullptr);
        }
    }

    m_ImageIndex = 0;
    m_FrameIndex = 0;

    VkSwapchainCreateInfoKHR SwapchainCreateInfo = {};
    SwapchainCreateInfo.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    SwapchainCreateInfo.compositeAlpha           = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    SwapchainCreateInfo.clipped                  = VK_TRUE;
    SwapchainCreateInfo.imageArrayLayers         = 1;
    SwapchainCreateInfo.surface                  = m_Surface;
    SwapchainCreateInfo.oldSwapchain             = OldSwapchain;

    auto Details                     = SwapchainSupportDetails::QuerySwapchainSupportDetails(m_Device->GetPhysicalDevice(), m_Surface);
    SwapchainCreateInfo.preTransform = Details.SurfaceCapabilities.currentTransform;

    m_SwapchainImageFormat              = Details.ChooseBestSurfaceFormat(Details.ImageFormats);
    SwapchainCreateInfo.imageColorSpace = m_SwapchainImageFormat.colorSpace;
    SwapchainCreateInfo.imageFormat     = m_SwapchainImageFormat.format;

    m_CurrentPresentMode            = Details.ChooseBestPresentMode(Details.PresentModes, Application::Get().GetWindow()->IsVSync());
    SwapchainCreateInfo.presentMode = m_CurrentPresentMode;

    ELS_ASSERT(Details.SurfaceCapabilities.maxImageCount > 0, "Swapchain max image count less than zero!");
    m_SwapchainImageCount             = std::clamp(Details.SurfaceCapabilities.minImageCount + 1, Details.SurfaceCapabilities.minImageCount,
                                                   Details.SurfaceCapabilities.maxImageCount);
    SwapchainCreateInfo.minImageCount = m_SwapchainImageCount;

    m_SwapchainImageExtent =
        Details.ChooseBestExtent(Details.SurfaceCapabilities, static_cast<GLFWwindow*>(Application::Get().GetWindow()->GetNativeWindow()));
    SwapchainCreateInfo.imageExtent = m_SwapchainImageExtent;

    SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (m_Device->GetQueueFamilyIndices().GraphicsFamily != m_Device->GetQueueFamilyIndices().PresentFamily)
    {
        uint32_t Indices[] = {m_Device->GetQueueFamilyIndices().GraphicsFamily, m_Device->GetQueueFamilyIndices().PresentFamily};
        SwapchainCreateInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        SwapchainCreateInfo.queueFamilyIndexCount = 2;
        SwapchainCreateInfo.pQueueFamilyIndices   = Indices;
    }
    else
    {
        SwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    VK_CHECK(vkCreateSwapchainKHR(m_Device->GetLogicalDevice(), &SwapchainCreateInfo, nullptr, &m_Swapchain),
             "Failed to create vulkan swapchain!");

    if (OldSwapchain) vkDestroySwapchainKHR(m_Device->GetLogicalDevice(), OldSwapchain, nullptr);

    VK_CHECK(vkGetSwapchainImagesKHR(m_Device->GetLogicalDevice(), m_Swapchain, &m_SwapchainImageCount, nullptr));
    ELS_ASSERT(m_SwapchainImageCount > 0, "Failed to retrieve swapchain images!");

    m_SwapchainImages.resize(m_SwapchainImageCount);
    VK_CHECK(vkGetSwapchainImagesKHR(m_Device->GetLogicalDevice(), m_Swapchain, &m_SwapchainImageCount, m_SwapchainImages.data()));

    m_SwapchainImageViews.resize(m_SwapchainImages.size());
    for (uint32_t i = 0; i < m_SwapchainImageViews.size(); ++i)
    {
        ImageUtils::CreateImageView(m_Device->GetLogicalDevice(), m_SwapchainImages[i], &m_SwapchainImageViews[i],
                                    m_SwapchainImageFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    // First call return.
    if (!m_DepthImage)
    {
        // Depth Buffer
        ImageSpecification DepthImageSpec;
        DepthImageSpec.Usage  = EImageUsage::Attachment;
        DepthImageSpec.Format = EImageFormat::DEPTH32F;
        DepthImageSpec.Height = m_SwapchainImageExtent.height;
        DepthImageSpec.Width  = m_SwapchainImageExtent.width;
        m_DepthImage.reset(new VulkanImage(DepthImageSpec));

        InvalidateRenderPass();

        return;
    }

    if (m_DepthImage)
    {
        m_DepthImage->SetExtent(m_SwapchainImageExtent);
        m_DepthImage->Create();

        InvalidateRenderPass();
    }
}

void VulkanSwapchain::Destroy()
{
    vkDestroySwapchainKHR(m_Device->GetLogicalDevice(), m_Swapchain, nullptr);

    for (auto& ImageView : m_SwapchainImageViews)
    {
        vkDestroyImageView(m_Device->GetLogicalDevice(), ImageView, nullptr);
    }

    m_DepthImage->Destroy();
    DestroyRenderPass();
}

void VulkanSwapchain::InvalidateRenderPass()
{
    // RenderPass creation
    {
        std::vector<VkAttachmentDescription> Attachments(2);
        Attachments[0].format         = VK_FORMAT_B8G8R8A8_UNORM /*m_Swapchain->GetImageFormat()*/;
        Attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
        Attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        Attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        Attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        Attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        Attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        Attachments[0].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        Attachments[1].format         = m_DepthImage->GetFormat();
        Attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
        Attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        Attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        Attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        Attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        Attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        Attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        std::vector<VkAttachmentReference> AttachmentRefs(2);
        AttachmentRefs[0].attachment = 0;
        AttachmentRefs[0].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        AttachmentRefs[1].attachment = 1;
        AttachmentRefs[1].layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        std::vector<VkSubpassDescription> Subpasses(1);
        Subpasses[0].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        Subpasses[0].colorAttachmentCount    = 1;
        Subpasses[0].pColorAttachments       = &AttachmentRefs[0];
        Subpasses[0].pDepthStencilAttachment = &AttachmentRefs[1];

        std::vector<VkSubpassDependency> Dependencies(1);
        Dependencies[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
        Dependencies[0].dstSubpass    = 0;
        Dependencies[0].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        Dependencies[0].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        Dependencies[0].srcAccessMask = 0;
        Dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo RenderPassCreateInfo = {};
        RenderPassCreateInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        RenderPassCreateInfo.dependencyCount        = static_cast<uint32_t>(Dependencies.size());
        RenderPassCreateInfo.pDependencies          = Dependencies.data();
        RenderPassCreateInfo.pSubpasses             = Subpasses.data();
        RenderPassCreateInfo.subpassCount           = static_cast<uint32_t>(Subpasses.size());
        RenderPassCreateInfo.pAttachments           = Attachments.data();
        RenderPassCreateInfo.attachmentCount        = static_cast<uint32_t>(Attachments.size());

        VK_CHECK(vkCreateRenderPass(m_Device->GetLogicalDevice(), &RenderPassCreateInfo, nullptr, &m_RenderPass),
                 "Failed to create swapchain render pass!");
    }

    // Framebuffers creation

    m_Framebuffers.resize(m_SwapchainImageCount);

    VkImageView Attachments[2] = {m_SwapchainImageViews[0], m_DepthImage->GetView()};

    VkFramebufferCreateInfo FramebufferCreateInfo = {};
    FramebufferCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FramebufferCreateInfo.width                   = m_SwapchainImageExtent.width;
    FramebufferCreateInfo.height                  = m_SwapchainImageExtent.height;
    FramebufferCreateInfo.layers                  = 1;
    FramebufferCreateInfo.pAttachments            = &Attachments[0];
    FramebufferCreateInfo.attachmentCount         = 2;
    FramebufferCreateInfo.renderPass              = m_RenderPass;

    for (uint32_t i = 0; i < m_SwapchainImageCount; ++i)
    {
        Attachments[0] = m_SwapchainImageViews[i];

        VK_CHECK(vkCreateFramebuffer(m_Device->GetLogicalDevice(), &FramebufferCreateInfo, nullptr, &m_Framebuffers[i]),
                 "Failed to create framebuffer!");
    }
}

void VulkanSwapchain::DestroyRenderPass()
{
    vkDestroyRenderPass(m_Device->GetLogicalDevice(), m_RenderPass, nullptr);

    for (auto& Framebuffer : m_Framebuffers)
        vkDestroyFramebuffer(m_Device->GetLogicalDevice(), Framebuffer, nullptr);
}

}  // namespace Eclipse
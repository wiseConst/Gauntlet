#include "GauntletPCH.h"
#include "VulkanSwapchain.h"

#include "VulkanUtility.h"
#include "VulkanDevice.h"
#include "VulkanContext.h"
#include "VulkanImage.h"

#include "Gauntlet/Core/Application.h"
#include "Gauntlet/Core/Window.h"
#include "SwapchainSupportDetails.h"

#include "Gauntlet/Core/Timer.h"

namespace Gauntlet
{

VulkanSwapchain::VulkanSwapchain(Scoped<VulkanDevice>& device, VkSurfaceKHR& surface) : m_Device(device), m_Surface(surface)
{
    Invalidate();

    m_CommandBuffers.resize(FRAMES_IN_FLIGHT);
    for (auto& commandBuffer : m_CommandBuffers)
    {
        commandBuffer = MakeRef<VulkanCommandBuffer>(ECommandBufferType::COMMAND_BUFFER_TYPE_GRAPHICS);
    }
}

void VulkanSwapchain::BeginRenderPass(const VkCommandBuffer& commandBuffer)
{
    std::vector<VkClearValue> ClearValues(2);
    ClearValues[0].color        = {m_ClearColor};
    ClearValues[1].depthStencil = {1.0f};

    VkRenderPassBeginInfo RenderPassBeginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    RenderPassBeginInfo.framebuffer           = m_Framebuffers[m_ImageIndex];
    RenderPassBeginInfo.pClearValues          = ClearValues.data();
    RenderPassBeginInfo.clearValueCount       = static_cast<uint32_t>(ClearValues.size());
    RenderPassBeginInfo.renderPass            = m_RenderPass;

    VkRect2D RenderArea            = {};
    RenderArea.offset              = {0, 0};
    RenderArea.extent              = m_SwapchainImageExtent;
    RenderPassBeginInfo.renderArea = RenderArea;

    vkCmdBeginRenderPass(commandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

bool VulkanSwapchain::TryAcquireNextImage(const VkSemaphore& imageAcquiredSemaphore, const VkFence& fence)
{
    const auto result =
        vkAcquireNextImageKHR(m_Device->GetLogicalDevice(), m_Swapchain, UINT64_MAX, imageAcquiredSemaphore, fence, &m_ImageIndex);
    if (result == VK_SUCCESS) return true;

    if (result != VK_SUBOPTIMAL_KHR || result != VK_ERROR_OUT_OF_DATE_KHR)
    {
        const auto ResultMessage =
            std::string("Failed to acquire image from the swapchain! Result is: ") + std::string(GetStringVulkanResult(result));
        GNT_ASSERT(false, ResultMessage.data());
    }

    Recreate();
    return false;
}

void VulkanSwapchain::PresentImage(const VkSemaphore& renderFinishedSemaphore)
{
    VkPresentInfoKHR presentInfo   = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &renderFinishedSemaphore;
    presentInfo.pImageIndices      = &m_ImageIndex;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &m_Swapchain;

    const float imagePresentBegin = static_cast<float>(Timer::Now());

    const auto result = vkQueuePresentKHR(m_Device->GetPresentQueue(), &presentInfo);

    const float imagePresentEnd      = static_cast<float>(Timer::Now());
    Renderer::GetStats().PresentTime = imagePresentEnd - imagePresentBegin;

    if (result == VK_SUCCESS)
    {
        m_FrameIndex = (m_FrameIndex + 1) % FRAMES_IN_FLIGHT;
        return;
    }

    Recreate();
}

void VulkanSwapchain::Invalidate()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

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

    VkSwapchainCreateInfoKHR SwapchainCreateInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
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

    GNT_ASSERT(Details.SurfaceCapabilities.maxImageCount > 0, "Swapchain max image count less than zero!");
    m_SwapchainImageCount             = std::clamp(Details.SurfaceCapabilities.minImageCount + 1, Details.SurfaceCapabilities.minImageCount,
                                                   Details.SurfaceCapabilities.maxImageCount);
    SwapchainCreateInfo.minImageCount = m_SwapchainImageCount;

    m_SwapchainImageExtent =
        Details.ChooseBestExtent(Details.SurfaceCapabilities, static_cast<GLFWwindow*>(Application::Get().GetWindow()->GetNativeWindow()));
    SwapchainCreateInfo.imageExtent = m_SwapchainImageExtent;

    SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (Details.SurfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        SwapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

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

    VK_CHECK(vkGetSwapchainImagesKHR(m_Device->GetLogicalDevice(), m_Swapchain, &m_SwapchainImageCount, nullptr),
             "Failed to retrieve swapchain images !");
    GNT_ASSERT(m_SwapchainImageCount > 0, );

    m_SwapchainImages.resize(m_SwapchainImageCount);
    VK_CHECK(vkGetSwapchainImagesKHR(m_Device->GetLogicalDevice(), m_Swapchain, &m_SwapchainImageCount, m_SwapchainImages.data()),
             "Failed to acquire swapchain images!");

    m_SwapchainImageViews.resize(m_SwapchainImages.size());
    for (uint32_t i = 0; i < m_SwapchainImageViews.size(); ++i)
    {
        ImageUtils::CreateImageView(m_Device->GetLogicalDevice(), m_SwapchainImages[i], &m_SwapchainImageViews[i],
                                    m_SwapchainImageFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    InvalidateRenderPass();
}

void VulkanSwapchain::Destroy()
{
    vkDestroySwapchainKHR(m_Device->GetLogicalDevice(), m_Swapchain, nullptr);

    for (auto& ImageView : m_SwapchainImageViews)
    {
        vkDestroyImageView(m_Device->GetLogicalDevice(), ImageView, nullptr);
    }

    DestroyRenderPass();

    m_CommandBuffers.clear();
}

void VulkanSwapchain::InvalidateRenderPass()
{
    if (m_RenderPass) DestroyRenderPass();

    // RenderPass creation
    {
        VkAttachmentDescription ColorAttachmentDesc = {};
        ColorAttachmentDesc.format                  = m_SwapchainImageFormat.format;
        ColorAttachmentDesc.samples                 = VK_SAMPLE_COUNT_1_BIT;
        ColorAttachmentDesc.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        ColorAttachmentDesc.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
        ColorAttachmentDesc.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        ColorAttachmentDesc.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        ColorAttachmentDesc.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
        ColorAttachmentDesc.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference ColorAttachmentRef = {};
        ColorAttachmentRef.attachment            = 0;
        ColorAttachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription SubpassDesc = {};
        SubpassDesc.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        SubpassDesc.colorAttachmentCount = 1;
        SubpassDesc.pColorAttachments    = &ColorAttachmentRef;

        VkSubpassDependency SubpassDependency = {};
        SubpassDependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
        SubpassDependency.dstSubpass          = 0;
        SubpassDependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        SubpassDependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        SubpassDependency.srcAccessMask       = 0;
        SubpassDependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo RenderPassCreateInfo = {};
        RenderPassCreateInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        RenderPassCreateInfo.dependencyCount        = 1;
        RenderPassCreateInfo.pDependencies          = &SubpassDependency;
        RenderPassCreateInfo.subpassCount           = 1;
        RenderPassCreateInfo.pSubpasses             = &SubpassDesc;
        RenderPassCreateInfo.attachmentCount        = 1;
        RenderPassCreateInfo.pAttachments           = &ColorAttachmentDesc;

        VK_CHECK(vkCreateRenderPass(m_Device->GetLogicalDevice(), &RenderPassCreateInfo, nullptr, &m_RenderPass),
                 "Failed to create swapchain render pass!");
    }

    m_Framebuffers.resize(m_SwapchainImageCount);
    VkImageView ColorAttachment                   = m_SwapchainImageViews[0];
    VkFramebufferCreateInfo FramebufferCreateInfo = {};
    FramebufferCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FramebufferCreateInfo.width                   = m_SwapchainImageExtent.width;
    FramebufferCreateInfo.height                  = m_SwapchainImageExtent.height;
    FramebufferCreateInfo.layers                  = 1;
    FramebufferCreateInfo.pAttachments            = &ColorAttachment;
    FramebufferCreateInfo.attachmentCount         = 1;
    FramebufferCreateInfo.renderPass              = m_RenderPass;

    for (uint32_t i = 0; i < m_SwapchainImageCount; ++i)
    {
        ColorAttachment = m_SwapchainImageViews[i];

        VK_CHECK(vkCreateFramebuffer(m_Device->GetLogicalDevice(), &FramebufferCreateInfo, nullptr, &m_Framebuffers[i]),
                 "Failed to create framebuffer!");
    }
}

void VulkanSwapchain::DestroyRenderPass()
{
    vkDestroyRenderPass(m_Device->GetLogicalDevice(), m_RenderPass, nullptr);
    m_RenderPass = nullptr;

    for (auto& Framebuffer : m_Framebuffers)
        vkDestroyFramebuffer(m_Device->GetLogicalDevice(), Framebuffer, nullptr);
}

void VulkanSwapchain::Recreate()
{
    if (Application::Get().GetWindow()->IsMinimized()) return;
    m_Device->WaitDeviceOnFinish();

    Invalidate();

    for (auto& OneResizeCallback : m_ResizeCallbacks)
        OneResizeCallback();

#if GNT_DEBUG
    LOG_WARN("Swapchain recreated with: (%u, %u).", GetImageExtent().width, GetImageExtent().height);
#endif
}

}  // namespace Gauntlet
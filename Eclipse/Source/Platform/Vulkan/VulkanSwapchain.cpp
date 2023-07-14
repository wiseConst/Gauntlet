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

    if (!m_DepthImage)
    {
        // Depth Buffer
        ImageSpecification DepthImageSpec;
        DepthImageSpec.Usage  = EImageUsage::Attachment;
        DepthImageSpec.Format = EImageFormat::DEPTH32F;
        DepthImageSpec.Height = m_SwapchainImageExtent.height;
        DepthImageSpec.Width  = m_SwapchainImageExtent.width;
        m_DepthImage.reset(new VulkanImage(DepthImageSpec));

        return;
    }

    if (m_DepthImage)
    {
        m_DepthImage->SetExtent(m_SwapchainImageExtent);
        m_DepthImage->Create();
    }
}

void VulkanSwapchain::Destroy()
{
    if (m_DepthImage) m_DepthImage->Destroy();

    vkDestroySwapchainKHR(m_Device->GetLogicalDevice(), m_Swapchain, nullptr);

    for (auto& ImageView : m_SwapchainImageViews)
    {
        vkDestroyImageView(m_Device->GetLogicalDevice(), ImageView, nullptr);
    }
}

}  // namespace Eclipse
#include "EclipsePCH.h"
#include "VulkanSwapchain.h"

#include "VulkanCore.h"
#include "VulkanDevice.h"
#include "VulkanContext.h"
#include "VulkanTexture.h"

#include "Eclipse/Core/Application.h"
#include "Eclipse/Core/Window.h"

#include "SwapchainSupportDetails.h"

namespace Eclipse
{
VulkanSwapchain::VulkanSwapchain(Scoped<VulkanDevice>& InDevice, VkSurfaceKHR& InSurface, const ImageSpecification& InImageSpecification)
    : m_Device(InDevice), m_Surface(InSurface), m_DepthImage(nullptr)
{
    // By impl
    if (InImageSpecification.ImageViewAspectFlags != VK_IMAGE_ASPECT_NONE)
    {
        m_DepthImage.reset(new VulkanImage(InImageSpecification));
    }
    Create();
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
    VkPresentInfoKHR PresentInfo = {};
    PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    PresentInfo.waitSemaphoreCount = 1;
    PresentInfo.pWaitSemaphores = &InRenderFinishedSemaphore;
    PresentInfo.pImageIndices = &m_ImageIndex;
    PresentInfo.swapchainCount = 1;
    PresentInfo.pSwapchains = &m_Swapchain;

    const auto result = vkQueuePresentKHR(m_Device->GetPresentQueue(), &PresentInfo);
    if (result == VK_SUCCESS)
    {
        m_FrameIndex = (m_FrameIndex + 1) % FRAMES_IN_FLIGHT;
        return true;
    }

    return false;
}

void VulkanSwapchain::Create()
{
    m_ImageIndex = 0;
    m_FrameIndex = 0;

    VkSwapchainCreateInfoKHR SwapchainCreateInfo = {};
    SwapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    SwapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    SwapchainCreateInfo.clipped = VK_TRUE;
    SwapchainCreateInfo.imageArrayLayers = 1;
    SwapchainCreateInfo.surface = m_Surface;
    SwapchainCreateInfo.oldSwapchain = m_OldSwapchain;

    auto Details = SwapchainSupportDetails::QuerySwapchainSupportDetails(m_Device->GetPhysicalDevice(), m_Surface);
    SwapchainCreateInfo.preTransform = Details.SurfaceCapabilities.currentTransform;

    m_SwapchainImageFormat = Details.ChooseBestSurfaceFormat(Details.ImageFormats);
    SwapchainCreateInfo.imageColorSpace = m_SwapchainImageFormat.colorSpace;
    SwapchainCreateInfo.imageFormat = m_SwapchainImageFormat.format;

    m_CurrentPresentMode = Details.ChooseBestPresentMode(Details.PresentModes, Application::Get().GetWindow()->IsVSync());
    SwapchainCreateInfo.presentMode = m_CurrentPresentMode;

    ELS_ASSERT(Details.SurfaceCapabilities.maxImageCount > 0, "Swapchain max image count less than zero!");
    m_SwapchainImageCount = std::clamp(Details.SurfaceCapabilities.minImageCount + 1, Details.SurfaceCapabilities.minImageCount,
                                       Details.SurfaceCapabilities.maxImageCount);
    SwapchainCreateInfo.minImageCount = m_SwapchainImageCount;

    m_SwapchainImageExtent =
        Details.ChooseBestExtent(Details.SurfaceCapabilities, static_cast<GLFWwindow*>(Application::Get().GetWindow()->GetNativeWindow()));
    SwapchainCreateInfo.imageExtent = m_SwapchainImageExtent;

    // VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT - This is a color image I'm rendering into.
    // VK_IMAGE_USAGE_TRANSFER_SRC_BIT - I'll be copying this image somewhere. ( screenshot, postprocess )
    SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT /* | VK_IMAGE_USAGE_TRANSFER_SRC_BIT*/;

    if (m_Device->GetQueueFamilyIndices().GetGraphicsFamily() != m_Device->GetQueueFamilyIndices().GetPresentFamily())
    {
        uint32_t Indices[] = {m_Device->GetQueueFamilyIndices().GetGraphicsFamily(), m_Device->GetQueueFamilyIndices().GetPresentFamily()};
        SwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        SwapchainCreateInfo.queueFamilyIndexCount = 2;
        SwapchainCreateInfo.pQueueFamilyIndices = Indices;
    }
    else
    {
        SwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        SwapchainCreateInfo.queueFamilyIndexCount = 0;
        SwapchainCreateInfo.pQueueFamilyIndices = nullptr;
    }

    VK_CHECK(vkCreateSwapchainKHR(m_Device->GetLogicalDevice(), &SwapchainCreateInfo, nullptr, &m_Swapchain),
             "Failed to create vulkan swapchain!");

    if (m_OldSwapchain)
    {
        for (uint32_t i = 0; i < m_SwapchainImageCount; ++i)
        {
            vkDestroyImageView(m_Device->GetLogicalDevice(), m_SwapchainImageViews[i], nullptr);
        }
        vkDestroySwapchainKHR(m_Device->GetLogicalDevice(), m_OldSwapchain, nullptr);
    }

    VK_CHECK(vkGetSwapchainImagesKHR(m_Device->GetLogicalDevice(), m_Swapchain, &m_SwapchainImageCount, nullptr));
    ELS_ASSERT(m_SwapchainImageCount > 0, "Failed to retrieve swapchain images!");

    m_SwapchainImages.resize(m_SwapchainImageCount);
    VK_CHECK(vkGetSwapchainImagesKHR(m_Device->GetLogicalDevice(), m_Swapchain, &m_SwapchainImageCount, m_SwapchainImages.data()));

    m_SwapchainImageViews.resize(m_SwapchainImages.size());
    for (uint32_t i = 0; i < m_SwapchainImageViews.size(); ++i)
    {
        VulkanImage::CreateImageView(m_Device->GetLogicalDevice(), m_SwapchainImages[i], &m_SwapchainImageViews[i],
                                     m_SwapchainImageFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    // Depth image creation
    m_DepthImage->SetExtent({m_SwapchainImageExtent.width, m_SwapchainImageExtent.height, 1});
    if (m_DepthImage) m_DepthImage->Create();
}

void VulkanSwapchain::Destroy()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    m_OldSwapchain = m_Swapchain;

    if (m_DepthImage) m_DepthImage->Destroy();

    if (Context.IsDestroying())
    {
        vkDestroySwapchainKHR(m_Device->GetLogicalDevice(), m_Swapchain, nullptr);

        for (auto& ImageView : m_SwapchainImageViews)
        {
            vkDestroyImageView(m_Device->GetLogicalDevice(), ImageView, nullptr);
        }
    }
}

}  // namespace Eclipse
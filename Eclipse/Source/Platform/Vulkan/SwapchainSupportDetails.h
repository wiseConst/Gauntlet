#pragma once

#include "Eclipse/Core/Core.h"
#include <GLFW/glfw3.h>

namespace Eclipse
{
struct SwapchainSupportDetails final
{
  public:
    static SwapchainSupportDetails QuerySwapchainSupportDetails(const VkPhysicalDevice& InPhysicalDevice, const VkSurfaceKHR& InSurface)
    {
        SwapchainSupportDetails Details = {};
        {
            const auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(InPhysicalDevice, InSurface, &Details.SurfaceCapabilities);
            ELS_ASSERT(result == VK_SUCCESS, "Failed to query surface capabilities.");
        }

        uint32_t SurfaceFormatNum{0};
        {
            const auto result = vkGetPhysicalDeviceSurfaceFormatsKHR(InPhysicalDevice, InSurface, &SurfaceFormatNum, nullptr);
            ELS_ASSERT(result == VK_SUCCESS && SurfaceFormatNum != 0, "Failed to query number of surface formats.");
        }

        {
            Details.ImageFormats.resize(SurfaceFormatNum);
            const auto result =
                vkGetPhysicalDeviceSurfaceFormatsKHR(InPhysicalDevice, InSurface, &SurfaceFormatNum, Details.ImageFormats.data());
            ELS_ASSERT(result == VK_SUCCESS && SurfaceFormatNum != 0, "Failed to query surface formats.");
        }

        uint32_t PresentModeCount{0};
        {
            const auto result = vkGetPhysicalDeviceSurfacePresentModesKHR(InPhysicalDevice, InSurface, &PresentModeCount, nullptr);
            ELS_ASSERT(result == VK_SUCCESS && PresentModeCount != 0, "Failed to query number of present modes.");
        }

        {
            Details.PresentModes.resize(PresentModeCount);
            const auto result =
                vkGetPhysicalDeviceSurfacePresentModesKHR(InPhysicalDevice, InSurface, &PresentModeCount, Details.PresentModes.data());
            ELS_ASSERT(result == VK_SUCCESS && PresentModeCount != 0, "Failed to query present modes.");
        }

        return Details;
    }

    static VkSurfaceFormatKHR ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& AvailableFormats)
    {
        // If Vulkan returned an unknown format, then just force what we want.
        if (AvailableFormats.size() == 1 && AvailableFormats[0].format == VK_FORMAT_UNDEFINED)
        {
            VkSurfaceFormatKHR Format = {};
            Format.format = VK_FORMAT_B8G8R8A8_SRGB;
            Format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
            return Format;
        }

        for (const auto& Format : AvailableFormats)
        {
            if (Format.format == VK_FORMAT_B8G8R8A8_SRGB && Format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return Format;
            }
        }

        LOG_WARN("Failed to choose best swapchain surface format.");
        return AvailableFormats[0];
    }

    static VkPresentModeKHR ChooseBestPresentMode(const std::vector<VkPresentModeKHR>& AvailablePresentModes, bool bIsVSync)
    {
        if (bIsVSync) return VK_PRESENT_MODE_FIFO_KHR;

        for (const auto& PresentMode : AvailablePresentModes)
        {
            if (PresentMode == VK_PRESENT_MODE_MAILBOX_KHR) return PresentMode;  // Battery-drain mode on mobile devices
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    static VkExtent2D ChooseBestExtent(const VkSurfaceCapabilitiesKHR& SurfaceCapabilities, GLFWwindow* window)
    {
        if (SurfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return SurfaceCapabilities.currentExtent;
        }

        int32_t Width{0}, Height{0};
        glfwGetFramebufferSize(window, &Width, &Height);

        VkExtent2D ActualExtent = {static_cast<uint32_t>(Width), static_cast<uint32_t>(Height)};
        ActualExtent.width =
            std::clamp(ActualExtent.width, SurfaceCapabilities.minImageExtent.width, SurfaceCapabilities.maxImageExtent.width);
        ActualExtent.height =
            std::clamp(ActualExtent.height, SurfaceCapabilities.minImageExtent.height, SurfaceCapabilities.maxImageExtent.height);

        return ActualExtent;
    }

  public:
    VkSurfaceCapabilitiesKHR SurfaceCapabilities;
    std::vector<VkPresentModeKHR> PresentModes;
    std::vector<VkSurfaceFormatKHR> ImageFormats;
};
}  // namespace Eclipse
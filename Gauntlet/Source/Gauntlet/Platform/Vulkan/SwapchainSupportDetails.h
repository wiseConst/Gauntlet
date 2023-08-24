#pragma once

#include "Gauntlet/Core/Core.h"
#include <GLFW/glfw3.h>

namespace Gauntlet
{
struct SwapchainSupportDetails final
{
  public:
    static SwapchainSupportDetails QuerySwapchainSupportDetails(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface)
    {
        SwapchainSupportDetails Details = {};
        {
            const auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &Details.SurfaceCapabilities);
            GNT_ASSERT(result == VK_SUCCESS, "Failed to query surface capabilities.");
        }

        uint32_t SurfaceFormatNum{0};
        {
            const auto result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &SurfaceFormatNum, nullptr);
            GNT_ASSERT(result == VK_SUCCESS && SurfaceFormatNum != 0, "Failed to query number of surface formats.");
        }

        {
            Details.ImageFormats.resize(SurfaceFormatNum);
            const auto result =
                vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &SurfaceFormatNum, Details.ImageFormats.data());
            GNT_ASSERT(result == VK_SUCCESS && SurfaceFormatNum != 0, "Failed to query surface formats.");
        }

        uint32_t PresentModeCount{0};
        {
            const auto result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &PresentModeCount, nullptr);
            GNT_ASSERT(result == VK_SUCCESS && PresentModeCount != 0, "Failed to query number of present modes.");
        }

        {
            Details.PresentModes.resize(PresentModeCount);
            const auto result =
                vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &PresentModeCount, Details.PresentModes.data());
            GNT_ASSERT(result == VK_SUCCESS && PresentModeCount != 0, "Failed to query present modes.");
        }

        return Details;
    }

    static VkSurfaceFormatKHR ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        // If Vulkan returned an unknown format, then just force what we want.
        if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
        {
            VkSurfaceFormatKHR Format = {};
            // ImGui uses VK_FORMAT_B8G8R8A8_UNORM
            Format.format     = VK_FORMAT_B8G8R8A8_UNORM;  // VK_FORMAT_B8G8R8A8_SRGB;
            Format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
            return Format;
        }

        for (const auto& Format : availableFormats)
        {
            // ImGui uses VK_FORMAT_B8G8R8A8_UNORM
            if (Format.format == VK_FORMAT_B8G8R8A8_UNORM /*VK_FORMAT_B8G8R8A8_SRGB*/ &&
                Format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return Format;
            }
        }

        LOG_WARN("Failed to choose best swapchain surface format.");
        return availableFormats[0];
    }

    static VkPresentModeKHR ChooseBestPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool bIsVSync)
    {
        if (bIsVSync) return VK_PRESENT_MODE_FIFO_KHR;

        for (const auto& PresentMode : availablePresentModes)
        {
            if (PresentMode == VK_PRESENT_MODE_MAILBOX_KHR) return PresentMode;  // Battery-drain mode on mobile devices
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    static VkExtent2D ChooseBestExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities, GLFWwindow* window)
    {
        if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return surfaceCapabilities.currentExtent;
        }

        int32_t Width{0}, Height{0};
        glfwGetFramebufferSize(window, &Width, &Height);

        VkExtent2D ActualExtent = {static_cast<uint32_t>(Width), static_cast<uint32_t>(Height)};
        ActualExtent.width =
            std::clamp(ActualExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        ActualExtent.height =
            std::clamp(ActualExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);

        return ActualExtent;
    }

  public:
    VkSurfaceCapabilitiesKHR SurfaceCapabilities;
    std::vector<VkPresentModeKHR> PresentModes;
    std::vector<VkSurfaceFormatKHR> ImageFormats;
};
}  // namespace Gauntlet
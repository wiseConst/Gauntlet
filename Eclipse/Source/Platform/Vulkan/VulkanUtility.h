#pragma once

#include <volk/volk.h>
#include <vector>

#include "Eclipse/Core/Core.h"

namespace Eclipse
{
#define LOG_VULKAN_INFO 0


constexpr uint32_t ELS_VK_API_VERSION = VK_API_VERSION_1_3;
constexpr uint32_t FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> VulkanLayers = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> DeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef ELS_DEBUG
constexpr bool bEnableValidationLayers = true;
#else
constexpr bool bEnableValidationLayers = false;
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    switch (messageSeverity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        {
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        {
            // LOG_INFO(pCallbackData->pMessage);
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        {
            LOG_WARN(pCallbackData->pMessage);
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        {
            LOG_ERROR(pCallbackData->pMessage);
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT: break;
    }

    return VK_FALSE;
}

}  // namespace Eclipse
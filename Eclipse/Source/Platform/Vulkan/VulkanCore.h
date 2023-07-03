#pragma once

#include "Eclipse/Core/Core.h"

#include <volk/volk.h>

namespace Eclipse
{

static const char* GetStringVulkanResult(VkResult InResult)
{
    switch (InResult)
    {
        case VK_SUCCESS: return "VK_SUCCESS";
        case VK_NOT_READY: return "VK_NOT_READY";
        case VK_TIMEOUT: return "VK_TIMEOUT";
        case VK_EVENT_SET: return "VK_EVENT_SET";
        case VK_EVENT_RESET: return "VK_EVENT_RESET";
        case VK_INCOMPLETE: return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";
    }

    ELS_ASSERT(false, "Unknown VkResult!");
    return nullptr;
}

#define VK_CHECK(x)                                                                                                                        \
    do                                                                                                                                     \
    {                                                                                                                                      \
        VkResult result = x;                                                                                                               \
        if (result != VK_SUCCESS)                                                                                                          \
        {                                                                                                                                  \
            ELS_ASSERT(false, "VkResult is: %s on #%u line.", GetStringVulkanResult(result), __LINE__);                                    \
        }                                                                                                                                  \
    } while (0);

#define VK_CHECK(x, message)                                                                                                               \
    do                                                                                                                                     \
    {                                                                                                                                      \
        VkResult result = x;                                                                                                               \
        std::string FinalMessage = std::string(message) + " The result is: " + std::string(GetStringVulkanResult(result));                 \
        if (result != VK_SUCCESS)                                                                                                          \
        {                                                                                                                                  \
            ELS_ASSERT(false, FinalMessage.data(), GetStringVulkanResult(result), __LINE__);                                               \
        }                                                                                                                                  \
    } while (0);

}  // namespace Eclipse
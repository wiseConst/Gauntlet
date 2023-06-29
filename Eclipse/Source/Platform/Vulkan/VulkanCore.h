#pragma once

#include "Eclipse/Core/Core.h"

#include <volk/volk.h>

namespace Eclipse
{

#define ELS_VK_STRING(x) (#x)

#define VK_CHECK(x)                                                                                                                        \
    do                                                                                                                                     \
    {                                                                                                                                      \
        VkResult result = x;                                                                                                               \
        if (result != VK_SUCCESS)                                                                                                          \
        {                                                                                                                                  \
            ELS_ASSERT(false, "VkResult is: %s on #%u line.", ELS_VK_STRING(result), __LINE__);                                            \
        }                                                                                                                                  \
    } while (0);

#define VK_CHECK(x, message)                                                                                                               \
    do                                                                                                                                     \
    {                                                                                                                                      \
        VkResult result = x;                                                                                                               \
        std::string FinalMessage = std::string(message) + " The result is: " + std::string(ELS_VK_STRING(result));                         \
        if (result != VK_SUCCESS)                                                                                                          \
        {                                                                                                                                  \
            ELS_ASSERT(false, FinalMessage.data(), ELS_VK_STRING(result), __LINE__);                                                       \
        }                                                                                                                                  \
    } while (0);

}  // namespace Eclipse
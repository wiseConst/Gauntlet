#include "GauntletPCH.h"
#include "Gauntlet/Core/Log.h"

#if GNT_DEBUG && LOG_VMA_INFO
#define VMA_DEBUG_LOG_FORMAT(format, ...)                                                                                                  \
    do                                                                                                                                     \
    {                                                                                                                                      \
        LOG_INFO((format), __VA_ARGS__);                                                                                                   \
    } while (false)
#else
#define VMA_DEBUG_LOG_FORMAT(format, ...)
#endif

// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/configuration.html#config_Vulkan_functions
// Since I'm using volk, and don't link to static lib, I have to disable static functions, but enable dynamic one.

#define VMA_DYNAMIC_VULKAN_FUNCTIONS VK_TRUE
#define VMA_STATIC_VULKAN_FUNCTIONS VK_FALSE
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

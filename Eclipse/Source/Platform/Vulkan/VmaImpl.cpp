#include "EclipsePCH.h"

#include "Eclipse/Core/Log.h"

// using namespace Eclipse;
//
//#ifdef ELS_DEBUG
//
//#def   ine  VMA_DEBUG_LOG_FORMAT(format, ...) \
//    do \
//    { \
//        LOG_WARN((format), __VA_ARGS__); \
//    } while (false)
//
//#else
//#define VMA_DEBUG_LOG_FORMAT(format, ...)
//#endif

#define VMA_DYNAMIC_VULKAN_FUNCTIONS VK_TRUE
#define VMA_STATIC_VULKAN_FUNCTIONS VK_FALSE
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

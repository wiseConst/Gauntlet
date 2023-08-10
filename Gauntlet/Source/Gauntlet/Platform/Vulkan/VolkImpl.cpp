#include "GauntletPCH.h"

#if defined(GNT_PLATFORM_WINDOWS)
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(GNT_PLATFORM_LINUX)
#define VK_USE_PLATFORM_XCB_KHR
#endif

#define VOLK_IMPLEMENTATION
#include <volk/volk.h>
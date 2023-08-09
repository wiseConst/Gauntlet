#pragma once

#include "PlatformDetection.h"
#include "Inherited.h"

#include <memory>

#ifdef GNT_PLATFORM_WINDOWS
#define NOMINMAX
#include <Windows.h>
#endif

#ifdef GNT_DEBUG
#define GNT_ENABLE_ASSERTS
#endif

#ifdef GNT_ENABLE_ASSERTS

#include <Gauntlet/Core/Log.h>
#define GNT_ASSERT(x, ...)                                                                                                                 \
    if (!(x))                                                                                                                              \
    {                                                                                                                                      \
        LOG_ERROR("Assertion failed: %s", __VA_ARGS__);                                                                                    \
        __debugbreak();                                                                                                                    \
    }

#else

#define GNT_ASSERT(x, ...) (x)

#endif

namespace Gauntlet
{
#define MAX_WORKER_THREADS 6

#define FORCEINLINE __forceinline
#define NODISCARD [[nodiscard]]
#define BIT(x) (1 << (x))
#define BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

// Stolen from vulkan_core.h
#define MAKE_VERSION(major, minor, patch) ((((uint32_t)(major)) << 22) | (((uint32_t)(minor)) << 12) | ((uint32_t)(patch)))

static constexpr uint32_t ApplicationVersion = MAKE_VERSION(1, 0, 0);
static constexpr uint32_t EngineVersion      = MAKE_VERSION(1, 0, 0);
static const char* EngineName                = "Gauntlet";

template <typename T> using Scoped = std::unique_ptr<T>;

template <typename T, typename... Args> constexpr Scoped<T> MakeScoped(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T> using Ref = std::shared_ptr<T>;

template <typename T, typename... Args> constexpr Ref<T> MakeRef(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename T> using Weak = std::weak_ptr<T>;

}  // namespace Gauntlet
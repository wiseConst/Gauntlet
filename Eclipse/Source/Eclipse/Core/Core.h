#pragma once

#include "PlatformDetection.h"
#include "Inherited.h"

#include <memory>

#ifdef ELS_PLATFORM_WINDOWS
#define NOMINMAX
#include <Windows.h>
#endif

#ifdef ELS_DEBUG
#define ELS_ENABLE_ASSERTS
#endif

#ifdef ELS_ENABLE_ASSERTS

#include <Eclipse/Core/Log.h>
#define ELS_ASSERT(x, ...)                                                                                                                 \
    if (!(x))                                                                                                                              \
    {                                                                                                                                      \
        LOG_ERROR("Assertion failed: %s", __VA_ARGS__);                                                                                    \
        __debugbreak();                                                                                                                    \
    }

#else

#define ELS_ASSERT(x, ...)

#endif

#define VECTOR_SIZE(vec) (sizeof((vec)[0]) * (vec).size())

#define FORCEINLINE __forceinline

#define BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

// Stolen from vulkan_core.h
#define MAKE_VERSION(major, minor, patch) ((((uint32_t)(major)) << 22) | (((uint32_t)(minor)) << 12) | ((uint32_t)(patch)))

namespace Eclipse
{
constexpr uint32_t ApplicationVersion = MAKE_VERSION(1, 0, 0);
constexpr uint32_t EngineVersion = MAKE_VERSION(1, 0, 0);
constexpr char* EngineName = "Eclipse";

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

template<typename T> using Weak = std::weak_ptr<T>;

}  // namespace Eclipse
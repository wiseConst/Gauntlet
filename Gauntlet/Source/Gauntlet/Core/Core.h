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

#ifdef GNT_PLATFORM_WINDOWS
#define GNT_DEBUG_BREAK() __debugbreak()
#elif GNT_PLATFORM_LINUX
#define GNT_DEBUG_BREAK() __builtin_debugtrap()
#elif GNT_PLATFORM_MACOS
#define GNT_DEBUG_BREAK() __builtin_trap()
#endif

#include <Gauntlet/Core/Log.h>
#define GNT_ASSERT(x, ...)                                                                                                                 \
    if (!(x))                                                                                                                              \
    {                                                                                                                                      \
        LOG_ERROR("Assertion failed: %s", __VA_ARGS__);                                                                                    \
        GNT_DEBUG_BREAK();                                                                                                                 \
    }

#else

#define GNT_ASSERT(x, ...) (x)

#endif

namespace Gauntlet
{

#define MESH_SHADING_TEST 0

static constexpr uint32_t FRAMES_IN_FLIGHT = 2;

#define MAX_WORKER_THREADS 8
#define GRAPHICS_PREFER_INTEGRATED_GPU 0

#define FORCEINLINE __forceinline
#define NODISCARD [[nodiscard]]
#define BIT(x) (1 << (x))
#define BIND_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

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
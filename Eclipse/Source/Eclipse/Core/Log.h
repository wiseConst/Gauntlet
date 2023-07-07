#pragma once

#include <Eclipse/Core/Core.h>

// From KOHI Vulkan-Engine Series.

#define LOG_WARN_ENABLED 1
#define LOG_INFO_ENABLED 1
#define LOG_DEBUG_ENABLED 1
#define LOG_TRACE_ENABLED 1

#if ELS_RELEASE
#define LOG_DEBUG_ENABLED 0
#define LOG_TRACE_ENABLED 0
#endif

namespace Eclipse
{

enum class ELogLevel : uint8_t
{
    LL_FATAL = 0,
    LL_ERROR,
    LL_WARN,
    LL_INFO,
    LL_DEBUG,
    LL_TRACE
};

class Log final
{
  public:
    // Currently off use
    static void Init();
    static void Shutdown();

    static void Output(ELogLevel InLogLevel, const char* InMessage, ...);
};

#define LOG_FATAL(message, ...) Eclipse::Log::Output(ELogLevel::LL_FATAL, message, ##__VA_ARGS__)
#define LOG_ERROR(message, ...) Eclipse::Log::Output(ELogLevel::LL_ERROR, message, ##__VA_ARGS__)

#if LOG_WARN_ENABLED
#define LOG_WARN(message, ...) Eclipse::Log::Output(ELogLevel::LL_WARN, message, ##__VA_ARGS__)
#else
#define LOG_WARN(message, ...)
#endif

#if LOG_INFO_ENABLED
#define LOG_INFO(message, ...) Eclipse::Log::Output(ELogLevel::LL_INFO, message, ##__VA_ARGS__)
#else
#define LOG_INFO(message, ...)
#endif

#if LOG_DEBUG_ENABLED
#define LOG_DEBUG(message, ...) Eclipse::Log::Output(ELogLevel::LL_DEBUG, message, ##__VA_ARGS__)
#else
#define LOG_DEBUG(message, ...)
#endif

#if LOG_TRACE_ENABLED
#define LOG_TRACE(message, ...) Eclipse::Log::Output(Eclipse::ELogLevel::LL_TRACE, message, ##__VA_ARGS__)
#else
#define LOG_TRACE(message, ...)
#endif
}  // namespace Eclipse
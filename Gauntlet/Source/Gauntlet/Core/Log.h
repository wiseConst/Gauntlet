#pragma once

#include <Gauntlet/Core/Core.h>
#include <mutex>

// From KOHI Vulkan-Engine Series.

#define LOG_WARN_ENABLED 1
#define LOG_TRACE_ENABLED 1

namespace Gauntlet
{

enum class ELogLevel : uint8_t
{
    LL_FATAL = 0,
    LL_ERROR,
    LL_WARN,
    LL_INFO,
    LL_TRACE
};

class Log final
{
  public:
    // Currently off use
    static void Init();
    static void Shutdown();

    static void Output(ELogLevel InLogLevel, const char* InMessage, ...);

  private:
    static std::mutex s_LogMutex;
};

#define LOG_INFO(message, ...) Gauntlet::Log::Output(Gauntlet::ELogLevel::LL_INFO, message, ##__VA_ARGS__)

#if GNT_DEBUG
#define LOG_FATAL(message, ...) Gauntlet::Log::Output(Gauntlet::ELogLevel::LL_FATAL, message, ##__VA_ARGS__)
#define LOG_ERROR(message, ...) Gauntlet::Log::Output(Gauntlet::ELogLevel::LL_ERROR, message, ##__VA_ARGS__)
#elif GNT_RELEASE
#define LOG_FATAL(message, ...)
#define LOG_ERROR(message, ...)
#endif

#if LOG_WARN_ENABLED
#define LOG_WARN(message, ...) Gauntlet::Log::Output(Gauntlet::ELogLevel::LL_WARN, message, ##__VA_ARGS__)
#else
#define LOG_WARN(message, ...)
#endif

#if LOG_TRACE_ENABLED
#define LOG_TRACE(message, ...) Gauntlet::Log::Output(Gauntlet::ELogLevel::LL_TRACE, message, ##__VA_ARGS__)
#else
#define LOG_TRACE(message, ...)
#endif
}  // namespace Gauntlet
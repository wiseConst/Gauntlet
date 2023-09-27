#pragma once

#include <Gauntlet/Core/Core.h>
#include <mutex>
#include <fstream>

namespace Gauntlet
{
#define LOG_WARN_ENABLED 1
#define LOG_TRACE_ENABLED 1

#define OUT_MESSAGE_LENGTH 32000
#define MAX_TIME_FORMAT_LENGTH 100

enum class ELogLevel : uint8_t
{
    LL_FATAL = 0,
    LL_ERROR,
    LL_WARN,
    LL_INFO,
    LL_TRACE,
    LL_DEBUG
};

class Log final
{
  public:
    static void Init();
    static void Shutdown();

    template <typename... Args> static void Output(ELogLevel logLevel, const char* message, Args&&... args)
    {
        std::scoped_lock m_Lock(s_LogMutex);
        s_Output                            = std::ofstream("Gauntlet.log", std::ios::out | std::ios::app);
        static const char* LevelStrings[]   = {"[FATAL]: ", "[ERROR]: ", "[WARN]: ", "[INFO]: ", "[TRACE]: ", "[DEBUG]: "};
        static const char* LogLevelColors[] = {
            "\033[31m", "\033[91m", "\033[93m", "\033[94m", "\033[97m", "\033[92m",
        };

        int32_t LevelIndex = 0;
        switch (logLevel)
        {
            case ELogLevel::LL_FATAL:
            {
                LevelIndex = 0;
                break;
            }
            case ELogLevel::LL_ERROR:
            {
                LevelIndex = 1;
                break;
            }
            case ELogLevel::LL_WARN:
            {
                LevelIndex = 2;
                break;
            }
            case ELogLevel::LL_INFO:
            {
                LevelIndex = 3;
                break;
            }
            case ELogLevel::LL_TRACE:
            {
                LevelIndex = 4;
                break;
            }
            case ELogLevel::LL_DEBUG:
            {
                LevelIndex = 5;
                break;
            }
        }

        char FormattedMessage[OUT_MESSAGE_LENGTH] = {0};
        sprintf(FormattedMessage, message, args...);

        // Get global UTC time
        const std::chrono::system_clock::time_point CurrentTimeUTC = std::chrono::system_clock::now();

        // Convert to local time
        const std::time_t CurrentTime   = std::chrono::system_clock::to_time_t(CurrentTimeUTC);
        const std::tm* CurrentLocalTime = std::localtime(&CurrentTime);

        // Format the time with timezone
        char FormattedTime[MAX_TIME_FORMAT_LENGTH] = {0};
        std::strftime(FormattedTime, MAX_TIME_FORMAT_LENGTH, "[%d:%m:%Y|%H:%M:%S]", CurrentLocalTime);

        char FinalOutMessage[OUT_MESSAGE_LENGTH] = {0};
        sprintf(FinalOutMessage, "%s %s%s", FormattedTime, LevelStrings[LevelIndex], FormattedMessage);
        printf("%s%s \033[0m\n", LogLevelColors[LevelIndex], FinalOutMessage);
        s_Output << FinalOutMessage;

        s_Output.close();
    }

  private:
    static std::mutex s_LogMutex;
    static std::ofstream s_Output;
};

#define LOG_INFO(message, ...) Gauntlet::Log::Output(Gauntlet::ELogLevel::LL_INFO, message, ##__VA_ARGS__)

#if GNT_DEBUG
#define LOG_FATAL(message, ...) Gauntlet::Log::Output(Gauntlet::ELogLevel::LL_FATAL, message, ##__VA_ARGS__)
#define LOG_ERROR(message, ...) Gauntlet::Log::Output(Gauntlet::ELogLevel::LL_ERROR, message, ##__VA_ARGS__)
#define LOG_DEBUG(message, ...) Gauntlet::Log::Output(Gauntlet::ELogLevel::LL_DEBUG, message, ##__VA_ARGS__)
#elif GNT_RELEASE
#define LOG_FATAL(message, ...)
#define LOG_ERROR(message, ...)
#define LOG_DEBUG(message, ...)
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
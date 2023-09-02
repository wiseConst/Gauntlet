#include <GauntletPCH.h>
#include "Log.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

namespace Gauntlet
{
#define OUT_MESSAGE_LENGTH 32000
#define MAX_TIME_FORMAT_LENGTH 100

std::mutex Log::s_LogMutex;
std::ofstream Log::s_Output;

void Log::Init()
{
    s_Output = std::ofstream("Gauntlet.log", std::ios::out | std::ios::trunc);
    if (!s_Output.is_open()) LOG_WARN("Failed to create/open log file! %s");
}

void Log::Shutdown()
{
    // Default console output color
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);

    s_Output.close();
}

void Log::Output(ELogLevel logLevel, const char* message, ...)
{
    std::scoped_lock m_Lock(s_LogMutex);
    static const char* LevelStrings[] = {"[FATAL]: ", "[ERROR]: ", "[WARN]: ", "[INFO]: ", "[TRACE]: "};

    auto hConsole      = GetStdHandle(STD_OUTPUT_HANDLE);
    int32_t LevelIndex = 0;
    switch (logLevel)
    {
        case ELogLevel::LL_FATAL:
        {
            LevelIndex = 0;
            SetConsoleTextAttribute(hConsole, 4);
            break;
        }
        case ELogLevel::LL_ERROR:
        {
            LevelIndex = 1;
            SetConsoleTextAttribute(hConsole, 5);
            break;
        }
        case ELogLevel::LL_WARN:
        {
            LevelIndex = 2;
            SetConsoleTextAttribute(hConsole, 14);
            break;
        }
        case ELogLevel::LL_INFO:
        {
            LevelIndex = 3;
            SetConsoleTextAttribute(hConsole, 9);
            break;
        }
        case ELogLevel::LL_TRACE:
        {
            LevelIndex = 4;
            SetConsoleTextAttribute(hConsole, 15);
            break;
        }
    }

    char FormattedMessage[OUT_MESSAGE_LENGTH] = {0};

    va_list args;
    va_start(args, message);
    vsnprintf(FormattedMessage, OUT_MESSAGE_LENGTH, message, args);
    va_end(args);

    // Get global UTC time
    std::chrono::system_clock::time_point CurrentTimeUTC = std::chrono::system_clock::now();

    // Convert to local time
    std::time_t CurrentTime   = std::chrono::system_clock::to_time_t(CurrentTimeUTC);
    std::tm* CurrentLocalTime = std::localtime(&CurrentTime);

    // Format the time with timezone
    char FormattedTime[MAX_TIME_FORMAT_LENGTH] = {0};
    std::strftime(FormattedTime, MAX_TIME_FORMAT_LENGTH, "[%d:%m:%Y|%H:%M:%S]", CurrentLocalTime);

    char FinalOutMessage[OUT_MESSAGE_LENGTH] = {0};
    sprintf(FinalOutMessage, "%s %s%s\n", FormattedTime, LevelStrings[LevelIndex], FormattedMessage);
    printf("%s", FinalOutMessage);
    s_Output << FinalOutMessage;

    // Default console output color
    SetConsoleTextAttribute(hConsole, 7);
}

}  // namespace Gauntlet
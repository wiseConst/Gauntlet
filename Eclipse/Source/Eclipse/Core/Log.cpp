#include <EclipsePCH.h>
#include "Log.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

namespace Eclipse
{
#define OUT_MESSAGE_LENGTH 32000

std::mutex Log::s_LogMutex;

void Log::Init() {}

void Log::Shutdown() {}

void Log::Output(ELogLevel InLogLevel, const char* InMessage, ...)
{
    std::scoped_lock m_Lock(s_LogMutex);
    constexpr char* LevelStrings[] = {"[FATAL]: ", "[ERROR]: ", "[WARN]: ", "[INFO]: ", "[DEBUG]: ", "[TRACE]: "};
    // const bool bIsError = InLogLevel < ELogLevel::LL_WARN;

    auto hConsole      = GetStdHandle(STD_OUTPUT_HANDLE);
    int32_t LevelIndex = 0;
    switch (InLogLevel)
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
        case ELogLevel::LL_DEBUG:
        {
            LevelIndex = 4;
            SetConsoleTextAttribute(hConsole, 11);
            break;
        }
        case ELogLevel::LL_TRACE:
        {
            LevelIndex = 5;
            SetConsoleTextAttribute(hConsole, 15);
            break;
        }
    }

    char FormattedMessage[OUT_MESSAGE_LENGTH] = {0};

    va_list args;
    va_start(args, InMessage);
    vsnprintf(FormattedMessage, OUT_MESSAGE_LENGTH, InMessage, args);
    va_end(args);

    char FinalOutMessage[OUT_MESSAGE_LENGTH] = {0};
    sprintf(FinalOutMessage, "%s%s\n", LevelStrings[LevelIndex], FormattedMessage);
    printf("%s", FinalOutMessage);

    // Default console output color
    SetConsoleTextAttribute(hConsole, 7);
}

}  // namespace Eclipse
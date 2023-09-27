#include <GauntletPCH.h>
#include "Log.h"

namespace Gauntlet
{
std::mutex Log::s_LogMutex;
std::ofstream Log::s_Output;

void Log::Init()
{
    s_Output = std::ofstream("Gauntlet.log", std::ios::out | std::ios::trunc);
    if (!s_Output.is_open()) LOG_WARN("Failed to create/open log file! %s");
}

void Log::Shutdown()
{
    s_Output.close();
}

}  // namespace Gauntlet
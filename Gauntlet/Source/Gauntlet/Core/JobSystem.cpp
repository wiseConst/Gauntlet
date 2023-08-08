#include "GauntletPCH.h"
#include "JobSystem.h"

namespace Gauntlet
{
uint32_t JobSystem::s_ThreadCount;
std::array<std::thread, MAX_WORKER_THREADS> JobSystem::s_Threads;

void JobSystem::Init()
{
    s_ThreadCount = std::thread::hardware_concurrency() - 1;
    if (s_ThreadCount > MAX_WORKER_THREADS)
    {
        LOG_INFO("You got: %u threads, capping at: %u.", s_ThreadCount, MAX_WORKER_THREADS);
        s_ThreadCount = MAX_WORKER_THREADS;
    }
    LOG_INFO("MainThread ID: %u", std::this_thread::get_id());
    LOG_INFO("JobSystem using %u threads.", s_ThreadCount);

    for (uint32_t i = 0; i < s_ThreadCount; ++i)
    {
    }
}

void JobSystem::Shutdown() {}

}  // namespace Gauntlet
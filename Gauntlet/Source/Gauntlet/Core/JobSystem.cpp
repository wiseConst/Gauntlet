#include "GauntletPCH.h"
#include "JobSystem.h"

namespace Gauntlet
{
uint32_t JobSystem::s_ThreadCount = 0;
std::array<Thread, MAX_WORKER_THREADS> JobSystem::s_Threads;

static bool s_bIsInitialized = false;

void JobSystem::Init()
{
    GNT_ASSERT(!s_bIsInitialized, "Job system already initialized!");
    s_bIsInitialized = true;
    LOG_INFO("MainThread ID: %u", std::this_thread::get_id());

    s_ThreadCount = std::thread::hardware_concurrency() - 1;
    if (s_ThreadCount > MAX_WORKER_THREADS)
    {
        LOG_INFO("You got: %u threads, capping at: %u.", s_ThreadCount, MAX_WORKER_THREADS);
        s_ThreadCount = MAX_WORKER_THREADS;
    }
    LOG_INFO("JobSystem using %u threads.", s_ThreadCount);
}

void JobSystem::Update()
{
    static bool bThreadsStarted = false;
    if (!bThreadsStarted)
    {
        for (uint32_t i = 0; i < s_ThreadCount; ++i)
        {
            s_Threads[i].Start();
        }
        bThreadsStarted = true;
    }

    using namespace std::chrono;

    bool bAreJobsDone{false};
    while (!bAreJobsDone)
    {
        bool bAreThreadsIdling{true};
        for (auto& OneThread : s_Threads)
        {
            if (!OneThread.IsIdle())
            {
                bAreThreadsIdling = false;
                break;
            }
        }

        if (!bAreThreadsIdling)
        {
            std::this_thread::sleep_for(1ms);
            continue;
        }

        bAreJobsDone = true;
    }
}

void JobSystem::Shutdown()
{
    for (auto& Thread : s_Threads)
    {
        Thread.Shutdown();
    }
}

}  // namespace Gauntlet
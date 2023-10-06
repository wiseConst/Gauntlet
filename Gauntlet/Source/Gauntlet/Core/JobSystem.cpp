#include "GauntletPCH.h"
#include "JobSystem.h"

namespace Gauntlet
{
static bool s_bIsInitialized = false;

void JobSystem::Init()
{
    GNT_ASSERT(!s_bIsInitialized, "Job system already initialized!");
    s_bIsInitialized = true;

#if !GNT_DEBUG
    LOG_INFO("MainThread ID: %u", std::this_thread::get_id());
#endif

    s_ThreadCount = std::thread::hardware_concurrency() - 1;
    if (s_ThreadCount > MAX_WORKER_THREADS)
    {
#if !GNT_DEBUG
        LOG_INFO("You have: %u threads, capping at: %u.", s_ThreadCount, MAX_WORKER_THREADS);
#endif
        s_ThreadCount = MAX_WORKER_THREADS;
    }
#if !GNT_DEBUG
    LOG_INFO("JobSystem using %u threads.", s_ThreadCount);
#endif

    for (uint32_t i = 0; i < s_ThreadCount; ++i)
    {
        s_Threads[i].Start();
        s_Threads[i].SetThreadAffinity(i);
    }
}

void JobSystem::Update()
{
    for (auto& thread : s_Threads)
        thread.Wait();

    if (s_PendingJobs.empty()) return;

    std::scoped_lock<std::mutex> Lock(s_QueueMutex);
    for (uint32_t i = 0; i < s_ThreadCount; ++i)
    {
        if (s_PendingJobs.empty()) break;

        if (!s_Threads[i].IsIdle()) continue;

        s_Threads[i].Submit(s_PendingJobs.front());
        s_PendingJobs.pop();
    }
}

void JobSystem::Wait()
{
    do
    {
        Update();
    } while (!s_PendingJobs.empty());
}

void JobSystem::Shutdown()
{
    for (auto& Thread : s_Threads)
        Thread.Shutdown();
}

}  // namespace Gauntlet
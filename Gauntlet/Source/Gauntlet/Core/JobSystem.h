#pragma once

#include "Gauntlet/Core/Core.h"
#include "Thread.h"

namespace Gauntlet
{
class JobSystem final : private Uncopyable, private Unmovable
{
  public:
    static void Init();
    static void Shutdown();

    static void Update();

    template <typename Func, typename... Args> static void Submit(Func&& InFunc, Args&&... InArgs)
    {
        Job Command = std::bind(std::forward<Func>(InFunc), std::forward<Args>(InArgs)...);
        // Check if our application runs in singlethreaded mode with no worker threads at all.
        if (s_ThreadCount == 0)
        {
            Command();
            return;
        }

        uint32_t IdlingThread = 0;
        size_t MinJobs        = UINT64_MAX;
        for (uint32_t i = 0; i < MAX_WORKER_THREADS; ++i)
        {
            const auto JobsCount = s_Threads[i].GetJobsCount();
            if (JobsCount < MinJobs)
            {
                MinJobs      = JobsCount;
                IdlingThread = i;
            }
        }
        s_Threads[IdlingThread].Submit(Command);
    }

  private:
    static uint32_t s_ThreadCount;
    static std::array<Thread, MAX_WORKER_THREADS> s_Threads;
};

}  // namespace Gauntlet
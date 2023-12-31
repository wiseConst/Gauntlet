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
    static void Wait();

    template <typename Func, typename... Args> static void Submit(Func&& InFunc, Args&&... args)
    {
        std::scoped_lock<std::mutex> Lock(s_QueueMutex);

        Job Command = std::bind(std::forward<Func>(InFunc), std::forward<Args>(args)...);
        // Check if our application runs in singlethreaded mode with no worker threads at all.
        if (s_ThreadCount == 0)
        {
            Command();
            return;
        }

        for (uint32_t i = 0; i < s_ThreadCount; ++i)
        {
            if (s_Threads[i].IsIdle())
            {
                s_Threads[i].Submit(Command);
                return;
            }
        }

        s_PendingJobs.emplace(Command);
    }

    FORCEINLINE static uint32_t GetThreadCount() { return s_ThreadCount; }
    FORCEINLINE static auto& GetThreads() { return s_Threads; }

  private:
    inline static uint32_t s_ThreadCount = 0;
    inline static std::array<Thread, MAX_WORKER_THREADS> s_Threads;

    inline static std::mutex s_QueueMutex;
    inline static std::queue<Job> s_PendingJobs;
};

}  // namespace Gauntlet
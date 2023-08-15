#pragma once

#include "Gauntlet/Core/Core.h"

namespace Gauntlet
{

using Job = std::function<void()>;

enum class EThreadState : uint8_t
{
    IDLE = 0,
    WORKING
};

class Thread final : private Uncopyable, private Unmovable
{
  public:
    Thread()  = default;
    ~Thread() = default;

    void Start();
    void Shutdown();

    FORCEINLINE void Join()
    {
        if (m_Handle.joinable()) m_Handle.join();
    }

    FORCEINLINE void Submit(const std::function<void()>& InJob)
    {
        {
            std::unique_lock<std::mutex> Lock(m_QueueMutex);
            m_Jobs.push(InJob);
        }

        m_CondVar.notify_one();
    }

    FORCEINLINE const size_t GetJobsCount() const { return m_Jobs.size(); }
    FORCEINLINE const bool IsIdle() const { return m_ThreadState == EThreadState::IDLE; }

  private:
    std::thread m_Handle;
    std::queue<Job> m_Jobs;
    std::mutex m_QueueMutex;
    std::condition_variable m_CondVar;

    EThreadState m_ThreadState  = EThreadState::IDLE;
    bool m_bIsShutdownRequested = false;

    friend class JobSystem;

    // Should be called only once on init
    void SetThreadAffinity(const uint32_t InThreadID);
};
}  // namespace Gauntlet
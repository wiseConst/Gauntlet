#pragma once

#include "EclipsePCH.h"
#include "Eclipse/Core/Core.h"

namespace Eclipse
{

class ThreadPool final : private Uncopyable, private Unmovable
{
    using Task = std::function<void()>;

  public:
    ThreadPool();
    ~ThreadPool();

    template <typename F, typename... Args> void Enqueue(F&& f, Args&&... InArgs)
    {
        {
            std::scoped_lock<std::mutex> Lock(m_Mutex);
            m_TaskQueue.emplace(std::bind(std::forward<F>(f), std::forward<Args>(InArgs)...));
        }

        m_CondVar.notify_one();
    }

  private:
    uint32_t m_ThreadCount;
    const std::thread::id m_MainId;
    std::vector<std::thread> m_Threads;

    std::condition_variable m_CondVar;
    std::mutex m_Mutex;
    std::queue<Task> m_TaskQueue;
    bool m_IsShutdownRequested;

    void Init();
    void Shutdown();
};
}  // namespace Eclipse
#include "EclipsePCH.h"
#include "ThreadPool.h"

namespace Eclipse
{
ThreadPool::ThreadPool() : m_MainId(std::this_thread::get_id())
{
    Init();

#if ELS_DEBUG
    LOG_INFO("ThreadPool created! Thread usage limit: %u", m_ThreadCount);
#endif
}

ThreadPool::~ThreadPool()
{
    Shutdown();

#if ELS_DEBUG
    LOG_INFO("ThreadPool destroyed!");
#endif
}

void ThreadPool::Init()
{
    m_ThreadCount = std::thread::hardware_concurrency() - 1;  // Without MainThread that uses our applicaiton
    m_IsShutdownRequested = false;

    for (uint32_t i = 0; i < m_ThreadCount; ++i)
    {
        m_Threads.emplace_back(
            [this]
            {
                if (std::this_thread::get_id() == m_MainId) return;

                while (true)
                {
                    Task task;
                    {
                        std::unique_lock<std::mutex> Lock(m_QueueMutex);
                        m_CondVar.wait(Lock, [this] { return m_IsShutdownRequested || !m_TaskQueue.empty(); });

                        if (m_IsShutdownRequested && m_TaskQueue.empty()) return;

                        task = std::move(m_TaskQueue.front());
                        m_TaskQueue.pop();
                    }
                    task();
                }
            });
    }
}

void ThreadPool::Shutdown()
{
    {
        std::unique_lock<std::mutex> Lock(m_QueueMutex);
        m_IsShutdownRequested = true;
    }

    m_CondVar.notify_all();
    for (auto& Thread : m_Threads)
    {
        if (Thread.joinable()) Thread.join();
    }
}

}  // namespace Eclipse
#include "GauntletPCH.h"
#include "Thread.h"

namespace Gauntlet
{

void Thread::Start()
{
    m_Handle = std::thread(
        [&]
        {
            while (true)
            {
                std::unique_lock<std::mutex> Lock(m_QueueMutex);
                m_ThreadState = EThreadState::IDLE;

                // Wait until we want to shutdown or we got some jobs.
                m_CondVar.wait(Lock, [this] { return !m_Jobs.empty() || m_bIsShutdownRequested; });

                if (m_Jobs.empty() && m_bIsShutdownRequested) break;

                m_ThreadState = EThreadState::WORKING;
                if (!m_Jobs.empty())
                {
                    Job job = m_Jobs.front();
                    job();

                    m_Jobs.pop();
                }
            }
        });
}

void Thread::Shutdown()
{
    {
        std::unique_lock<std::mutex> Lock(m_QueueMutex);
        m_bIsShutdownRequested = true;
    }
    m_CondVar.notify_one();

    Join();
}

}  // namespace Gauntlet
#include "GauntletPCH.h"
#include "Thread.h"

#ifdef GNT_PLATFORM_WINDOWS
#include <Windows.h>
#endif

namespace Gauntlet
{

void Thread::Start()
{
    m_Handle = std::thread(
        [&]
        {
            while (true)
            {
                Job job;
                {
                    m_ThreadState = EThreadState::IDLE;
                    std::unique_lock<std::mutex> Lock(m_QueueMutex);

                    // Wait until we want to shutdown or we got some jobs.
                    m_CondVar.wait(Lock, [this] { return !m_Jobs.empty() || m_bIsShutdownRequested; });

                    if (m_Jobs.empty() && m_bIsShutdownRequested) break;

                    job = m_Jobs.front();

                    m_ThreadState = EThreadState::WORKING;
                    job();

                    m_Jobs.pop();
                    m_CondVar.notify_one();  // in case we got more than 1 job
                }
            }
        });
}

void Thread::Shutdown()
{
    {
        std::unique_lock<std::mutex> Lock(m_QueueMutex);
        m_bIsShutdownRequested = true;
        m_CondVar.notify_one();
    }

    Join();
}

void Thread::SetThreadAffinity(const uint32_t threadID)
{
#ifdef GNT_PLATFORM_WINDOWS

    // Attaching thread to specific CPU
    const HANDLE NativeHandle      = m_Handle.native_handle();
    const DWORD_PTR AffinityMask   = 1ull << threadID;
    const DWORD_PTR AffinityResult = SetThreadAffinityMask(NativeHandle, AffinityMask);
    GNT_ASSERT(AffinityResult > 0, "Failed to attach the thread to specific CPU core!");

    // Setting high priority to the thread
    // By default,each thread we create is THREAD_PRIORITY_DEFAULT.
    // Modifying this could help threads not be overtaken by the operating system by lesser priority threads.
    // I’ve found no way to increase performance with this yet, only decrease it.
    const BOOL PriorityResult = SetThreadPriority(NativeHandle, THREAD_PRIORITY_HIGHEST);
    GNT_ASSERT(PriorityResult != 0, "Failed to set thread priority to THREAD_PRIORITY_HIGHEST");

#endif
}

void Thread::Wait()
{
    std::unique_lock<std::mutex> lock(m_QueueMutex);
    m_CondVar.wait(lock, [this] { return m_Jobs.empty(); });
}

}  // namespace Gauntlet
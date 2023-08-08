#pragma once

#include "Gauntlet/Core/Core.h"

namespace Gauntlet
{

#define MAX_WORKER_THREADS 8

class JobSystem final : private Uncopyable, private Unmovable
{

  public:
    static void Init();
    static void Shutdown();

  private:
    static uint32_t s_ThreadCount;
    static std::array<std::thread, MAX_WORKER_THREADS> s_Threads;
};

}  // namespace Gauntlet
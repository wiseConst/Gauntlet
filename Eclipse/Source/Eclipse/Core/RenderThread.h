#pragma once

#include "Eclipse/Core/Core.h"
#include <thread>

namespace Eclipse
{
enum class ERenderThreadState : uint8_t
{
    STOP = 0,
    RENDER,
    WAIT
};

class RenderThread final
{
  public:
    static void Start();
    static void Stop();
    static void Run();
    static void Wait();
    static void WaitForUpdate();

    static void WaitAndRender();

  private:
    std::thread m_Thread;

    ERenderThreadState m_State = ERenderThreadState::STOP;
    static RenderThread s_Instance;
};

}  // namespace Eclipse
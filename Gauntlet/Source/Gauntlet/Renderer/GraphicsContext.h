#pragma once

#include <Gauntlet/Core/Core.h>
#include "RendererAPI.h"

namespace Gauntlet
{

class Window;

class GraphicsContext : private Uncopyable, private Unmovable
{
  public:
    GraphicsContext() = delete;
    GraphicsContext(Scoped<Window>& window) : m_Window(window) {}
    virtual ~GraphicsContext() { s_Context = nullptr; }

    virtual void BeginRender() = 0;
    virtual void EndRender()   = 0;

    virtual void SwapBuffers()          = 0;
    virtual void SetVSync(bool bIsVSync) = 0;
    virtual void Destroy()              = 0;

    static GraphicsContext* Create(Scoped<Window>& window);

    FORCEINLINE static auto& Get() { return *s_Context; }

  protected:
    Scoped<Window>& m_Window;

    static GraphicsContext* s_Context;
};

}  // namespace Gauntlet
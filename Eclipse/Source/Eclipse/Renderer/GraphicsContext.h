#pragma once

#include <Eclipse/Core/Core.h>
#include "RendererAPI.h"

namespace Eclipse
{

class Window;

class GraphicsContext : private Uncopyable, private Unmovable
{
  public:
    GraphicsContext() = delete;
    GraphicsContext(Scoped<Window>& InWindow) : m_Window(InWindow) {}
    virtual ~GraphicsContext() { s_Context = nullptr; }

    virtual void BeginRender() = 0;
    virtual void EndRender()   = 0;

    virtual void SwapBuffers()          = 0;
    virtual void SetVSync(bool IsVSync) = 0;
    virtual void Destroy()              = 0;

    static GraphicsContext* Create(Scoped<Window>& InWindow);

    FORCEINLINE static auto& Get() { return *s_Context; }

  protected:
    Scoped<Window>& m_Window;

    static GraphicsContext* s_Context;
};

}  // namespace Eclipse
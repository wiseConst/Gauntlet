#pragma once

#include <Eclipse/Core/Core.h>

namespace Eclipse
{

class Window;

struct RenderStats
{
    RenderStats() { CPUWaitTime = GPUWaitTime = 0.0f; }

    float CPUWaitTime;
    float GPUWaitTime;
};

class GraphicsContext
{
  public:
    GraphicsContext(Scoped<Window>& InWindow) : m_Window(InWindow) {}
    virtual ~GraphicsContext() = default;

    GraphicsContext() = delete;
    GraphicsContext(const GraphicsContext& that) = delete;
    GraphicsContext(GraphicsContext&& that) = delete;

    GraphicsContext& operator=(const GraphicsContext& that) = delete;
    GraphicsContext& operator=(GraphicsContext&& that) = delete;

    virtual void BeginRender() = 0;
    virtual void EndRender() = 0;

    virtual void Destroy() = 0;

    static GraphicsContext* Create(Scoped<Window>& InWindow);
    static RenderStats& GetStats() { return s_RenderStats; }

     /*   template <typename T> 
        static Scoped<T>& Get() 
        { 
            return Scoped<T>(static_cast<GraphicsContext*>(s_Context.release())); 
        }*/

  protected:
    Scoped<Window>& m_Window;

    //static Scoped<GraphicsContext> s_Context;
    static RenderStats s_RenderStats;
    


};

}  // namespace Eclipse
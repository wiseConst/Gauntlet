#pragma once

#include "RendererAPI.h"
#include <glm/glm.hpp>

namespace Eclipse
{

class Renderer : private Uncopyable, private Unmovable
{
  public:
    Renderer()  = default;
    ~Renderer() = default;

    static void Init(RendererAPI::EAPI GraphicsAPI);
    static void Shutdown();

  private:
    static Renderer* s_Renderer;

  protected:
    virtual void Create()  = 0;
    virtual void Destroy() = 0;
};

}  // namespace Eclipse

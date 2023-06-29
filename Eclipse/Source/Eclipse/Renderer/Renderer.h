#pragma once

#include "RendererAPI.h"

namespace Eclipse
{

class Renderer : private Uncopyable, private Unmovable
{
  public:
    Renderer() = delete;
    ~Renderer() = delete;

    static void Init(RendererAPI::EAPI GraphicsAPI);
    static void Shutdown();
};

}  // namespace Eclipse

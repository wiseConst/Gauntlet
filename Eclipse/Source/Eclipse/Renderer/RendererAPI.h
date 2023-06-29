#pragma once

#include <Eclipse/Core/Core.h>

namespace Eclipse
{

class RendererAPI
{
  public:
    enum class EAPI : uint8_t
    {
        None = 0,
        Vulkan
    };

    FORCEINLINE static void Init(EAPI GraphicsAPI)
    {
        ELS_ASSERT(s_API == EAPI::None, "You can't initalize RHI twice!");
        s_API = GraphicsAPI;
    }

    static EAPI Get() { return s_API; }

  private:
    static EAPI s_API;
};

}  // namespace Eclipse
#pragma once

#include <Gauntlet/Core/Core.h>

namespace Gauntlet
{

class RendererAPI
{
  public:
    enum class EAPI : uint8_t
    {
        None = 0,
        Vulkan
    };

    FORCEINLINE static void Init(EAPI InGraphicsAPI)
    {
        if (InGraphicsAPI == s_API && s_API != EAPI::None) return;

        GNT_ASSERT(s_API == EAPI::None, "You can't initalize RHI twice!");
        s_API = InGraphicsAPI;
    }

    static EAPI Get() { return s_API; }

  private:
    static EAPI s_API;
};

}  // namespace Gauntlet
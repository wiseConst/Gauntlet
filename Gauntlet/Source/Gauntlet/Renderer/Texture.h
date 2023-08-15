#pragma once

#include "Gauntlet/Core/Core.h"
#include "Image.h"

namespace Gauntlet
{

class Texture2D
{
  public:
    Texture2D()          = default;
    virtual ~Texture2D() = default;

    virtual const uint32_t GetWidth() const  = 0;
    virtual const uint32_t GetHeight() const = 0;

    virtual void Destroy()                         = 0;
    FORCEINLINE virtual void* GetTextureID() const = 0;

    static Ref<Texture2D> Create(const std::string_view& TextureFilePath, const bool InbCreateTextureID = false);
};

}  // namespace Gauntlet
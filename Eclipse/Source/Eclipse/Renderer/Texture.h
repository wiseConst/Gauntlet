#pragma once

#include "Eclipse/Core/Core.h"
#include "Image.h"

namespace Eclipse
{

class Texture2D
{
  public:
    Texture2D() = default;
    virtual ~Texture2D() = default;

    virtual const uint32_t GetWidth() const = 0;
    virtual const uint32_t GetHeight() const = 0;

    virtual void Destroy() = 0;

    static Ref<Texture2D> Create(const std::string_view& TextureFilePath);
};

}  // namespace Eclipse
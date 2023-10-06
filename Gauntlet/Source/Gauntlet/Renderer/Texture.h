#pragma once

#include "Gauntlet/Core/Core.h"
#include "Image.h"

namespace Gauntlet
{

struct TextureSpecification
{
    EImageFormat Format   = EImageFormat::RGBA;
    ETextureWrap Wrap     = ETextureWrap::REPEAT;
    ETextureFilter Filter = ETextureFilter::LINEAR;
    bool CreateTextureID  = false;  // Can be used in shaders?
    bool GenerateMips     = false;
};

class Texture2D
{
  public:
    Texture2D()          = default;
    virtual ~Texture2D() = default;

    virtual const uint32_t GetWidth() const  = 0;
    virtual const uint32_t GetHeight() const = 0;

    virtual void Destroy()                                       = 0;
    FORCEINLINE virtual void* GetTextureID() const               = 0;
    FORCEINLINE virtual TextureSpecification& GetSpecification() = 0;

    static Ref<Texture2D> Create(const std::string_view& textureFilePath, const TextureSpecification& textureSpecification);
    static Ref<Texture2D> Create(const void* data, const size_t size, const uint32_t imageWidth, const uint32_t imageHeight,
                                 const TextureSpecification& textureSpecification = TextureSpecification());
};

}  // namespace Gauntlet
#pragma once

#include "Eclipse/Core/Core.h"
#include <stb/stb_image.h>

namespace Eclipse
{
enum class EImageFormat
{
    NONE = 0,
    RGB,
    RGBA,  // UNORM
    SRGB,

    DEPTH32F,
    DEPTH24STENCIL8
};

enum class EAnisotropyLevel : uint32_t
{
    X2 = 2,
    X4 = 4,
    X8 = 8,
    X16 = 16
};

enum class EImageUsage
{
    NONE = 0,
    TEXTURE,
    Attachment
};

enum class ELoadImageType : uint8_t
{
    GREY = 0,
    GREY_ALPHA,
    RGB,
    RGB_ALPHA,
};

enum class ETextureWrap
{
    NONE = 0,
    CLAMP,
    REPEAT
};

enum class ETextureFilter
{
    None = 0,
    LINEAR,
    NEAREST
};

enum class ETextureBlend : uint32_t
{
    NONE = 0,
    MIN,
    MAX
};

struct ImageSpecification
{
  public:
    uint32_t Width = 1;
    uint32_t Height = 1;
    uint32_t Mips = 1;
    uint32_t Layers = 1;

    EImageFormat Format = EImageFormat::RGBA;
    EImageUsage Usage = EImageUsage::TEXTURE;
    ETextureWrap Wrap = ETextureWrap::REPEAT;
    ETextureFilter Filter = ETextureFilter::LINEAR;

    EAnisotropyLevel AnisotropyLevel = EAnisotropyLevel::X4;

    bool Copyable = false;
    bool Comparable = false;
};

class Image
{
  public:
    Image() = default;
    ~Image() = default;

    FORCEINLINE virtual uint32_t GetWidth() const = 0;
    FORCEINLINE virtual uint32_t GetHeight() const = 0;
    FORCEINLINE virtual float GetAspectRatio() const = 0;

    FORCEINLINE virtual const ImageSpecification& GetSpecification() = 0;

    static Ref<Image> Create(const ImageSpecification& InImageSpecification, const std::string_view& InFilePath);

    virtual void Destroy() = 0;
};

namespace ImageUtils
{
FORCEINLINE bool IsDepthFormat(EImageFormat InImageFormat)
{
    switch (InImageFormat)
    {
        case EImageFormat::DEPTH32F: return true;
        case EImageFormat::DEPTH24STENCIL8: return true;
    }

    return false;
}

stbi_uc* LoadImageFromFile(const std::string_view& InFilePath, int32_t* OutWidth, int32_t* OutHeight, int32_t* OutChannels,
                           ELoadImageType InLoadImageType = ELoadImageType::RGB_ALPHA);

FORCEINLINE void UnloadImage(stbi_uc* InPixels)
{
    stbi_image_free(InPixels);
}
}  // namespace ImageUtils

}  // namespace Eclipse
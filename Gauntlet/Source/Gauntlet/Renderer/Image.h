#pragma once

#include "Gauntlet/Core/Core.h"
#include "CoreRendererTypes.h"
#include <stb/stb_image.h>
#include <unordered_map>

namespace Gauntlet
{
enum class EImageFormat : uint8_t
{
    NONE = 0,
    RGB,
    RGB16F,
    RGB32F,
    RGBA,
    SRGB,
    RGBA16F,
    RGBA32F,
    R8,
    R16,
    R16F,
    R32F,

    R11G11B10,

    DEPTH32F,
    DEPTH24STENCIL8
};

enum class EAnisotropyLevel : uint32_t
{
    X2  = 2,
    X4  = 4,
    X8  = 8,
    X16 = 16
};

enum class EImageUsage : uint8_t
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

enum class ETextureWrap : uint8_t
{
    NONE = 0,
    CLAMP_TO_BORDER,
    CLAMP_TO_EDGE,
    REPEAT
};

enum class ETextureFilter : uint8_t
{
    NONE = 0,
    LINEAR,
    NEAREST
};

enum class EImageTiling : uint8_t
{
    NONE = 0,
    OPTIMAL,
    LINEAR
};

struct ImageSpecification
{
  public:
    uint32_t Width  = 1;
    uint32_t Height = 1;
    uint32_t Mips   = 1;
    uint32_t Layers = 1;

    EImageFormat Format              = EImageFormat::RGBA;
    EImageUsage Usage                = EImageUsage::Attachment;
    ETextureWrap Wrap                = ETextureWrap::REPEAT;
    ETextureFilter Filter            = ETextureFilter::LINEAR;
    EImageTiling Tiling              = EImageTiling::OPTIMAL;
    EAnisotropyLevel AnisotropyLevel = EAnisotropyLevel::X4;

    bool Copyable        = false;
    bool Comparable      = false;
    bool FlipOnLoad      = false;
    bool CreateTextureID = false;
};

class Image
{
  public:
    Image()          = default;
    virtual ~Image() = default;

    virtual void Invalidate() = 0;
    virtual void Destroy()    = 0;

    FORCEINLINE virtual uint32_t GetWidth() const              = 0;
    FORCEINLINE virtual uint32_t GetHeight() const             = 0;
    FORCEINLINE virtual float GetAspectRatio() const           = 0;
    FORCEINLINE virtual ImageSpecification& GetSpecification() = 0;
    FORCEINLINE virtual void* GetTextureID() const             = 0;

    static Ref<Image> Create(const ImageSpecification& imageSpecification);
};

namespace ImageUtils
{

FORCEINLINE bool IsDepthFormat(EImageFormat imageFormat)
{
    switch (imageFormat)
    {
        case EImageFormat::DEPTH32F:
        case EImageFormat::DEPTH24STENCIL8: return true;
    }

    return false;
}

stbi_uc* LoadImageFromFile(const std::string_view& filePath, int32_t* outWidth, int32_t* outHeight, int32_t* outChannels,
                           const bool bFlipOnLoad = false, ELoadImageType loadImageType = ELoadImageType::RGB_ALPHA);

FORCEINLINE void UnloadImage(stbi_uc* pixels)
{
    stbi_image_free(pixels);
    pixels = nullptr;
}
}  // namespace ImageUtils

struct SamplerSpecification
{
    ETextureFilter MagFilter  = ETextureFilter::NONE;
    ETextureFilter MinFilter  = ETextureFilter::NONE;
    ETextureFilter MipmapMode = ETextureFilter::NONE;
    ETextureWrap AddressModeU = ETextureWrap::NONE;
    ETextureWrap AddressModeV = ETextureWrap::NONE;
    ETextureWrap AddressModeW = ETextureWrap::NONE;
    float MipLodBias          = 0.0f;
    bool AnisotropyEnable     = false;
    float MaxAnisotropy       = 0.0f;
    bool CompareEnable        = false;
    ECompareOp CompareOp      = ECompareOp::COMPARE_OP_NEVER;
    float MinLod              = 0.0f;
    float MaxLod              = 0.0f;
};

struct Sampler
{
    Sampler()  = default;
    ~Sampler() = default;

    void* Handle            = nullptr;
    uint32_t ImageUsedCount = 0;
};

class SamplerStorage : private Uncopyable, private Unmovable
{
  public:
    SamplerStorage()          = default;
    virtual ~SamplerStorage() = default;

    static void Initialize();
    static void Destroy();

    static const auto& CreateSampler(const SamplerSpecification& samplerSpec)
    {
        GNT_ASSERT(s_Samplers.size() + 1 < s_MaxSamplerCount, "Max sampler count limit reached!");
        GNT_ASSERT(s_Instance, "Sampler storage is not valid!");
        return s_Instance->CreateSamplerImpl(samplerSpec);
    }

    static void DecrementSamplerRef(void* handle)
    {
        for (auto& sampler : s_Samplers)
        {
            if (sampler.second.Handle == handle)
                if (--sampler.second.ImageUsedCount == 0) s_Instance->DestroySamplerImpl(handle);
        }
    }

  protected:
    struct SamplerKeyHash
    {
        size_t operator()(const SamplerSpecification& samplerSpec) const
        {
            return std::hash<size_t>()(static_cast<size_t>(samplerSpec.MagFilter)) +
                   std::hash<size_t>()(static_cast<size_t>(samplerSpec.MinFilter)) +
                   std::hash<size_t>()(static_cast<size_t>(samplerSpec.AddressModeU)) +
                   std::hash<size_t>()(static_cast<size_t>(samplerSpec.MaxAnisotropy));
        }
    };

    struct SamplerKeyEqual
    {
        bool operator()(const SamplerSpecification& lhs, const SamplerSpecification& rhs) const
        {
            return lhs.MinFilter == rhs.MinFilter &&  //
                   lhs.MagFilter == rhs.MagFilter &&  //

                   lhs.AddressModeU == rhs.AddressModeU &&  //
                   lhs.AddressModeV == rhs.AddressModeV &&  //
                   lhs.AddressModeW == rhs.AddressModeW &&  //

                   lhs.AnisotropyEnable == rhs.AnisotropyEnable &&  //
                   lhs.MaxAnisotropy == rhs.MaxAnisotropy &&        //

                   lhs.CompareEnable == rhs.CompareEnable &&  //
                   lhs.CompareOp == rhs.CompareOp &&          //

                   lhs.MipmapMode == rhs.MipmapMode &&  //
                   lhs.MipLodBias == rhs.MipLodBias &&  //
                   lhs.MinLod == rhs.MinLod &&          //
                   lhs.MaxLod == rhs.MaxLod;
        }
    };

    static inline uint32_t s_MaxSamplerCount = 0;
    static inline SamplerStorage* s_Instance = nullptr;
    static inline std::unordered_map<SamplerSpecification, Sampler, SamplerKeyHash, SamplerKeyEqual> s_Samplers;

    virtual void InitializeImpl()                                                     = 0;
    virtual const Sampler& CreateSamplerImpl(const SamplerSpecification& samplerSpec) = 0;
    virtual void DestroyImpl()                                                        = 0;
    virtual void DestroySamplerImpl(void* handle)                                     = 0;
};

}  // namespace Gauntlet
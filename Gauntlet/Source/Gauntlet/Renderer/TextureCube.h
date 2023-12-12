#pragma once

#include "Gauntlet/Core/Core.h"
#include <vector>
#include "Image.h"

namespace Gauntlet
{

struct TextureCubeSpecification
{
    EImageFormat Format   = EImageFormat::RGBA;
    ETextureWrap Wrap     = ETextureWrap::REPEAT;
    ETextureFilter Filter = ETextureFilter::LINEAR;
    bool CreateTextureID  = false;  // Can be used in shaders?
};

class TextureCube : private Uncopyable, private Unmovable
{
  public:
    TextureCube()          = default;
    virtual ~TextureCube() = default;

    virtual void Destroy() = 0;

    static Ref<TextureCube> Create(const TextureCubeSpecification& textureCubeSpec);
    static Ref<TextureCube> Create(const std::vector<std::string>& faces);
};
}  // namespace Gauntlet
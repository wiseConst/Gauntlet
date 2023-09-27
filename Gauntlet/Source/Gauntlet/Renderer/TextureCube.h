#pragma once

#include "Gauntlet/Core/Core.h"
#include <vector>

// TODO: Also add texture properties

namespace Gauntlet
{
class TextureCube : private Uncopyable, private Unmovable
{
  public:
    TextureCube()  = default;
    ~TextureCube() = default;

    virtual void Destroy() = 0;

    static Ref<TextureCube> Create(const std::vector<std::string>& faces);
};
}  // namespace Gauntlet
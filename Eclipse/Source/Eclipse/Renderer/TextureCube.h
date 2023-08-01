#pragma once

#include "Eclipse/Core/Core.h"
#include <vector>
#include <string_view>

namespace Eclipse
{
class TextureCube : private Uncopyable, private Unmovable
{
  public:
    TextureCube()  = default;
    ~TextureCube() = default;

    virtual void Destroy() = 0;

    static Ref<TextureCube> Create(const std::vector<std::string>& InFaces);
};
}  // namespace Eclipse
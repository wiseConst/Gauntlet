#pragma once

#include "Gauntlet/Core/Core.h"

namespace Gauntlet
{
class Shader
{
  public:
    Shader()          = default;
    virtual ~Shader() = default;

    static Ref<Shader> CreateShader(const std::string_view& InFilePath);
};

}  // namespace Gauntlet

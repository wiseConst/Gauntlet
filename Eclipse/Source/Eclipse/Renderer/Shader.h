#pragma once

#include "Eclipse/Core/Core.h"

namespace Eclipse
{
class Shader
{
  public:
    Shader()          = default;
    virtual ~Shader() = default;

    static Ref<Shader> CreateShader(const std::string_view& InFilePath);
};

}  // namespace Eclipse

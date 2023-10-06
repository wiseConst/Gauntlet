#pragma once

#include "Gauntlet/Core/Core.h"

#pragma warning(disable : 4834)

namespace Gauntlet
{

enum class EShaderStage : uint8_t
{
    SHADER_STAGE_VERTEX = 0,
    SHADER_STAGE_GEOMETRY,
    SHADER_STAGE_FRAGMENT,
    SHADER_STAGE_COMPUTE
};

class Texture2D;
class Image;
struct AllocatedBuffer;

class Shader
{
  public:
    Shader()          = default;
    virtual ~Shader() = default;

    virtual void Destroy() = 0;
    // TODO: Fill these gaps virtual void SetInt()

    virtual void Set(const std::string& name, const Ref<Texture2D>& texture)                                                       = 0;
    virtual void Set(const std::string& name, const Ref<Image>& image)                                                             = 0;
    virtual void Set(const std::string& name, const AllocatedBuffer& buffer, const uint64_t bufferSize, const uint64_t offset = 0) = 0;

    static Ref<Shader> Create(const std::string_view& filePath);
};

class ShaderLibrary final : private Uncopyable, private Unmovable
{
  public:
    static void Init() {}
    static void Shutdown()
    {
        for (auto& shader : s_LoadedShaders)
            shader.second->Destroy();

        s_LoadedShaders.clear();
    }

    // TODO: Make it Load("FlatColor"), but not Load("Assets/Shaders/FlatColor")
    static Ref<Shader> Load(const std::string& shaderFilePath)
    {
        const size_t pos             = shaderFilePath.find_last_of('/');
        const std::string shaderName = shaderFilePath.substr(pos + 1, shaderFilePath.size() - pos);

        s_LoadedShaders[shaderName] = Shader::Create(shaderFilePath);
        return s_LoadedShaders[shaderName];
    }

    FORCEINLINE static Ref<Shader> Get(const std::string& shaderName)
    {
        GNT_ASSERT(s_LoadedShaders.contains(shaderName), "ShaderLibrary doesn't have %s", shaderName.data());
        return s_LoadedShaders[shaderName];
    }

  private:
    static inline std::unordered_map<std::string, Ref<Shader>> s_LoadedShaders;
};

}  // namespace Gauntlet

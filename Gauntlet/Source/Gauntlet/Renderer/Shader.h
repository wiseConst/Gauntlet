#pragma once

#include "Gauntlet/Core/Core.h"
#include "Buffer.h"

#pragma warning(disable : 4834)

namespace Gauntlet
{

enum class EShaderStage : uint8_t
{
    SHADER_STAGE_VERTEX = 0,
    SHADER_STAGE_GEOMETRY,
    SHADER_STAGE_FRAGMENT,

    SHADER_STAGE_TESSELLATION_CONTROL,
    SHADER_STAGE_TESSELLATION_EVALUATION,

    SHADER_STAGE_COMPUTE,

    SHADER_STAGE_RAYGEN,
    SHADER_STAGE_MISS,
    SHADER_STAGE_CLOSEST_HIT,
    SHADER_STAGE_ANY_HIT,

    SHADER_STAGE_TASK,
    SHADER_STAGE_MESH
};

class Texture2D;
class TextureCube;
class Image;
class UniformBuffer;

class Shader
{
  public:
    Shader()          = default;
    virtual ~Shader() = default;

    virtual BufferLayout GetVertexBufferLayout() = 0;
    virtual void Destroy()                       = 0;

    virtual void Set(const std::string& name, const Ref<Texture2D>& texture)                                      = 0;
    virtual void Set(const std::string& name, const Ref<TextureCube>& texture)                                    = 0;
    virtual void Set(const std::string& name, const Ref<Image>& image)                                            = 0;
    virtual void Set(const std::string& name, const Ref<UniformBuffer>& uniformBuffer, const uint64_t offset = 0) = 0;
    virtual void Set(const std::string& name, const Ref<StorageBuffer>& ssbo, const uint64_t offset = 0)          = 0;
    virtual void Set(const std::string& name, const std::vector<Ref<Texture2D>>& textures)                        = 0;

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

    static Ref<Shader> Load(const std::string& shaderName)
    {
        s_LoadedShaders[shaderName] = Shader::Create(shaderName);
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

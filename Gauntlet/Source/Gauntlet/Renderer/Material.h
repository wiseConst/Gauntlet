#pragma once

#include "Gauntlet/Core/Core.h"
#include "CoreRendererTypes.h"

namespace Gauntlet
{

class Texture2D;
class UniformBuffer;

class Material : private Unmovable
{
  public:
    Material()          = default;
    virtual ~Material() = default;

    virtual void Invalidate() = 0;
    virtual void Destroy()    = 0;

    virtual void Update() = 0;

    virtual FORCEINLINE const void* GetDescriptorSet() const = 0;
    virtual FORCEINLINE void* GetDescriptorSet()             = 0;

    FORCEINLINE Ref<Texture2D> GetAlbedo() { return m_AlbedoTextures.empty() ? nullptr : m_AlbedoTextures[0]; }
    FORCEINLINE Ref<Texture2D> GetNormalMap() { return m_NormalTextures.empty() ? nullptr : m_NormalTextures[0]; }
    FORCEINLINE Ref<Texture2D> GetMetallic() { return m_MetallicTextures.empty() ? nullptr : m_MetallicTextures[0]; }
    FORCEINLINE Ref<Texture2D> GetRoughness() { return m_RougnessTextures.empty() ? nullptr : m_RougnessTextures[0]; }
    FORCEINLINE Ref<Texture2D> GetAO() { return m_AOTextures.empty() ? nullptr : m_AOTextures[0]; }
    PBRMaterial& GetData() { return m_Data; }

    static Ref<Material> Create();

  protected:
    std::vector<Ref<Texture2D>> m_AlbedoTextures;
    std::vector<Ref<Texture2D>> m_NormalTextures;
    std::vector<Ref<Texture2D>> m_MetallicTextures;
    std::vector<Ref<Texture2D>> m_RougnessTextures;
    std::vector<Ref<Texture2D>> m_AOTextures;

    PBRMaterial m_Data;
    std::array<Ref<UniformBuffer>, FRAMES_IN_FLIGHT> m_UBMaterial;

    friend class Mesh;
};

}  // namespace Gauntlet
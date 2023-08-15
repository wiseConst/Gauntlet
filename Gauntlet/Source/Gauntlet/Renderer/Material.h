#pragma once

#include "Gauntlet/Core/Core.h"

namespace Gauntlet
{
class Texture2D;

// No need to move it imho, but can be copied
class Material : private Unmovable
{
  public:
    Material()  = default;
    ~Material() = default;

    virtual void Invalidate() = 0;

    static Ref<Material> Create();

    FORCEINLINE void SetDiffuseTextures(const std::vector<Ref<Texture2D>>& InDiffuseTextures) { m_DiffuseTextures = InDiffuseTextures; }
    FORCEINLINE void SetNormalMapTextures(const std::vector<Ref<Texture2D>>& InNormalMapTextures)
    {
        m_NormalMapTextures = InNormalMapTextures;
    }
    FORCEINLINE void SetEmissiveTextures(const std::vector<Ref<Texture2D>>& InEmissiveTextures) { m_EmissiveTextures = InEmissiveTextures; }

    FORCEINLINE Ref<Texture2D> GetDiffuseTexture(const uint32_t InTextureIndex)
    {
        return m_DiffuseTextures.empty() ? nullptr : m_DiffuseTextures[InTextureIndex];
    }

    FORCEINLINE Ref<Texture2D> GetNormalMapTexture(const uint32_t InTextureIndex)
    {
        return m_NormalMapTextures.empty() ? nullptr : m_NormalMapTextures[InTextureIndex];
    }

    FORCEINLINE Ref<Texture2D> GetEmissiveTexture(const uint32_t InTextureIndex)
    {
        return m_EmissiveTextures.empty() ? nullptr : m_EmissiveTextures[InTextureIndex];
    }

  protected:
    std::vector<Ref<Texture2D>> m_DiffuseTextures;
    std::vector<Ref<Texture2D>> m_NormalMapTextures;
    std::vector<Ref<Texture2D>> m_EmissiveTextures;
};

}  // namespace Gauntlet
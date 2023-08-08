#pragma once

#include "Gauntlet/Renderer/Renderer2D.h"

namespace Gauntlet
{

class Texture2D;
class VulkanContext;

class VulkanRenderer2D final : public Renderer2D
{
  public:
    VulkanRenderer2D();
    ~VulkanRenderer2D() = default;

    void Create() final override;
    void Destroy() final override;

    void BeginImpl() final override;
    void FlushImpl() final override;

    void BeginSceneImpl(const Camera& InCamera) final override;

    void DrawQuadImpl(const glm::vec3& InPosition, const glm::vec2& InSize, const glm::vec4& InColor) final override;

    void DrawQuadImpl(const glm::vec3& InPosition, const glm::vec2& InSize, const glm::vec3& InRotation, const Ref<Texture2D>& InTexture,
                      const glm::vec4& InColor) final override;

    void DrawRotatedQuadImpl(const glm::vec3& InPosition, const glm::vec2& InSize, const glm::vec3& InRotation,
                             const glm::vec4& InColor) final override;

    void DrawTexturedQuadImpl(const glm::vec3& InPosition, const glm::vec2& InSize, const Ref<Texture2D>& InTexture,
                              const glm::vec4& InBlendColor) final override;

  private:
    VulkanContext& m_Context;

    void FlushAndReset();
    void PreallocateRenderStorage();
};

}  // namespace Gauntlet
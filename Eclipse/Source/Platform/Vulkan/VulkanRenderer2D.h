#pragma once

#include "Eclipse/Renderer/Renderer2D.h"

#include "VulkanContext.h"

namespace Eclipse
{
class VulkanRenderer2D final : public Renderer2D
{
  public:
    VulkanRenderer2D();
    ~VulkanRenderer2D() = default;

    void Create() final override;
    void Destroy() final override;

    void BeginImpl() final override;
    void FlushImpl() final override;

    void BeginSceneImpl(const OrthographicCamera& InCamera) final override;

    void SetClearColorImpl(const glm::vec4& InColor) final override;

    void DrawQuadImpl(const glm::vec3& InPosition, const glm::vec2& InSize, const glm::vec4& InColor) final override;

    void DrawRotatedQuadImpl(const glm::vec3& InPosition, const glm::vec2& InSize, const glm::vec3& InRotation,
                             const glm::vec4& InColor) final override;

  private:
    VulkanContext& m_Context;
};

}  // namespace Eclipse
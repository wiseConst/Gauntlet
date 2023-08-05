#pragma once

#include "Eclipse/Renderer/Renderer.h"

namespace Eclipse
{

class VulkanContext;

class VulkanRenderer final : public Renderer
{
  public:
    VulkanRenderer();
    ~VulkanRenderer() = default;

    void Create() final override;
    void Destroy() final override;

    void BeginSceneImpl(const PerspectiveCamera& InCamera) final override;
    void SubmitMeshImpl(const Ref<Mesh>& InMesh, const glm::mat4& InTransformMatrix) final override;

    void BeginImpl() final override;
    void FlushImpl() final override;

  private:
    VulkanContext& m_Context;

    void SetupSkybox();
    void DrawSkybox();
    void DestroySkybox();
};

}  // namespace Eclipse
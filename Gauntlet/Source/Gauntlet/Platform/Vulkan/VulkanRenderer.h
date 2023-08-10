#pragma once

#include "Gauntlet/Renderer/Renderer.h"

#include <volk/volk.h>

namespace Gauntlet
{

class VulkanFramebuffer;
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

    static const Ref<VulkanFramebuffer>& GetPostProcessFramebuffer();
    static const VkDescriptorSetLayout& GetImageDescriptorSetLayout();

    const Ref<Image> GetFinalImageImpl() final override;

  private:
    VulkanContext& m_Context;

    void SetupSkybox();
    void DrawSkybox();
    void DestroySkybox();
};

}  // namespace Gauntlet
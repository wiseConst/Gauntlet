#pragma once

#include "Gauntlet/Renderer/Renderer.h"

#include <volk/volk.h>

#include "VulkanBuffer.h"
#include "Gauntlet/Renderer/CoreRendererStructs.h"

namespace Gauntlet
{

class VulkanFramebuffer;
class VulkanContext;
class VulkanPipeline;
class VulkanShader;
class VulkanTexture2D;
class VulkanCommandBuffer;
class Skybox;

class VulkanRenderer final : public Renderer
{
  private:
    struct VulkanRendererStorage
    {
        // Framebuffers && RenderPasses
        Ref<VulkanFramebuffer> PostProcessFramebuffer = nullptr;

        // Mesh
        Ref<VulkanPipeline> MeshPipeline      = nullptr;
        Ref<VulkanShader> MeshVertexShader    = nullptr;
        Ref<VulkanShader> MeshFragmentShader  = nullptr;
        Ref<VulkanTexture2D> MeshWhiteTexture = nullptr;

        BufferLayout MeshVertexBufferLayout;

        VkDescriptorSetLayout MeshDescriptorSetLayout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> MeshDescriptorSets;
        uint32_t CurrentDescriptorSetIndex = 0;

        // Camera UBO
        std::vector<AllocatedBuffer> UniformCameraDataBuffers;
        std::vector<void*> MappedUniformCameraDataBuffers;
        CameraDataBuffer MeshCameraDataBuffer;

        // Skybox
        Ref<Skybox> DefaultSkybox              = nullptr;
        Ref<VulkanPipeline> SkyboxPipeline     = nullptr;
        Ref<VulkanShader> SkyboxVertexShader   = nullptr;
        Ref<VulkanShader> SkyboxFragmentShader = nullptr;

        BufferLayout SkyboxVertexBufferLayout;
        VkDescriptorSetLayout SkyboxDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSet SkyboxDescriptorSet             = VK_NULL_HANDLE;
        MeshPushConstants SkyboxPushConstants;

        // UI
        VkDescriptorSetLayout ImageDescriptorSetLayout = VK_NULL_HANDLE;

        // Misc
        VulkanCommandBuffer* CurrentCommandBuffer = nullptr;
        Ref<VulkanPipeline> MeshWireframePipeline = nullptr;

        // Light
        std::vector<AllocatedBuffer> UniformPhongModelBuffers;
        std::vector<void*> MappedUniformPhongModelBuffers;
        PhongModelBuffer MeshPhongModelBuffer;
    };

  public:
    VulkanRenderer();
    ~VulkanRenderer() = default;

    void Create() final override;
    void Destroy() final override;

    void ApplyPhongModelImpl(const glm::vec4& LightPosition, const glm::vec4& LightColor,
                             const glm::vec3& AmbientSpecularShininess) final override;

    void BeginSceneImpl(const PerspectiveCamera& InCamera) final override;
    void SubmitMeshImpl(const Ref<Mesh>& InMesh, const glm::mat4& InTransformMatrix) final override;

    void BeginImpl() final override;
    void FlushImpl() final override;

    const Ref<Image> GetFinalImageImpl() final override;

    FORCEINLINE static VulkanRendererStorage& GetStorageData() { return s_Data; }

  private:
    VulkanContext& m_Context;

    inline static VulkanRendererStorage s_Data;

    void SetupSkybox();
    void DrawSkybox();
    void DestroySkybox();
};

}  // namespace Gauntlet
#pragma once

#include "Gauntlet/Renderer/Renderer.h"

#include <volk/volk.h>

#include "VulkanBuffer.h"
#include "VulkanDescriptors.h"
#include "Gauntlet/Renderer/CoreRendererStructs.h"

namespace Gauntlet
{

class VulkanFramebuffer;
class VulkanContext;
class VulkanPipeline;
class VulkanShader;
class VulkanTexture2D;
class VulkanCommandBuffer;
class VulkanMaterial;
class Skybox;

class VulkanRenderer final : public Renderer
{
  private:
    struct GeometryData
    {
        Ref<Gauntlet::VulkanMaterial> Material;
        Ref<Gauntlet::VulkanVertexBuffer> VulkanVertexBuffer;
        Ref<Gauntlet::VulkanIndexBuffer> VulkanIndexBuffer;
        glm::mat4 Transform;
    };

    struct VulkanRendererStorage
    {
        // Deffered rendering
        Ref<VulkanFramebuffer> DefferedFramebuffer;

        // Framebuffers && RenderPasses
        Ref<VulkanFramebuffer> GeometryFramebuffer{nullptr};
        Ref<VulkanFramebuffer> ShadowMapFramebuffer{nullptr};
        Ref<VulkanFramebuffer> SetupFramebuffer{nullptr};  // Clear pass

        bool bFramebuffersNeedResize  = {false};
        glm::uvec2 NewFramebufferSize = {1280, 720};

        // Pipelines
        Ref<VulkanPipeline> ShadowMapPipeline = nullptr;
        Ref<VulkanPipeline> GeometryPipeline  = nullptr;

        // Mesh
        Ref<VulkanTexture2D> MeshWhiteTexture = nullptr;

        BufferLayout MeshVertexBufferLayout;
        VkDescriptorSetLayout MeshDescriptorSetLayout = VK_NULL_HANDLE;

        // Camera UBO
        std::vector<AllocatedBuffer> UniformCameraDataBuffers;
        std::vector<void*> MappedUniformCameraDataBuffers;
        CameraDataBuffer MeshCameraDataBuffer;

        // Shadows UBO
        std::vector<AllocatedBuffer> UniformShadowsBuffers;
        std::vector<void*> MappedUniformShadowsBuffers;
        ShadowsBuffer MeshShadowsBuffer;

        // Skybox
        Ref<Skybox> DefaultSkybox          = nullptr;
        Ref<VulkanPipeline> SkyboxPipeline = nullptr;

        BufferLayout SkyboxVertexBufferLayout;
        VkDescriptorSetLayout SkyboxDescriptorSetLayout = VK_NULL_HANDLE;
        DescriptorSet SkyboxDescriptorSet;
        MeshPushConstants SkyboxPushConstants;

        // UI
        VkDescriptorSetLayout ImageDescriptorSetLayout = VK_NULL_HANDLE;

        // Misc
        VulkanCommandBuffer* CurrentCommandBuffer  = nullptr;
        Ref<VulkanPipeline> DebugShadowMapPipeline = nullptr;
        VkDescriptorSetLayout DebugShadowMapDescriptorSetLayout;
        DescriptorSet DebugShadowMapDescriptorSet;
        std::vector<GeometryData> SortedGeometry;

        // Light UBO
        std::vector<AllocatedBuffer> UniformPhongModelBuffers;
        std::vector<void*> MappedUniformPhongModelBuffers;
        LightingModelBuffer MeshLightingModelBuffer;
        uint32_t CurrentPointLightIndex = 0;
    };

  public:
    VulkanRenderer();
    ~VulkanRenderer() = default;

    void Create() final override;
    void Destroy() final override;

    void AddPointLightImpl(const glm::vec3& position, const glm::vec3& color, const glm::vec3& AmbientSpecularShininess,
                           const glm::vec3& CLQ) final override;
    void AddDirectionalLightImpl(const glm::vec3& color, const glm::vec3& direction,
                                 const glm::vec3& AmbientSpecularShininess) final override;

    void BeginSceneImpl(const PerspectiveCamera& camera) final override;
    void EndSceneImpl() final override;
    void SubmitMeshImpl(const Ref<Mesh>& mesh, const glm::mat4& transform) final override;

    void BeginImpl() final override;
    void FlushImpl() final override;
    void ResizeFramebuffersImpl(uint32_t width, uint32_t height) final override;

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
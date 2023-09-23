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
        // Framebuffers && RenderPasses
        Ref<VulkanFramebuffer> ShadowMapFramebuffer = nullptr;
        Ref<VulkanFramebuffer> GeometryFramebuffer  = nullptr;
        Ref<VulkanFramebuffer> SetupFramebuffer     = nullptr;  // Clear pass
        Ref<VulkanFramebuffer> LightingFramebuffer  = nullptr;
        Ref<VulkanFramebuffer> SSAOFramebuffer      = nullptr;

        bool bFramebuffersNeedResize  = {false};
        glm::uvec2 NewFramebufferSize = {1280, 720};

        // Deffered stuff
        DescriptorSet LightingSet;
        VkDescriptorSetLayout LightingDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout DefferedDescriptorSetLayout = VK_NULL_HANDLE;

        // Pipelines
        Ref<VulkanPipeline> ShadowMapPipeline = nullptr;
        Ref<VulkanPipeline> GeometryPipeline  = nullptr;
        Ref<VulkanPipeline> DefferedPipeline  = nullptr;
        Ref<VulkanPipeline> LightingPipeline  = nullptr;
        Ref<VulkanPipeline> SSAOPipeline      = nullptr;

        // Mesh
        Ref<VulkanTexture2D> MeshWhiteTexture         = nullptr;
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
        VulkanCommandBuffer* CurrentCommandBuffer = nullptr;
        std::vector<GeometryData> SortedGeometry;

        // SamplerHandling
        struct SamplerKeyHash
        {
            size_t operator()(const VkSamplerCreateInfo& samplerCreateInfo) const
            {
                return std::hash<uint32_t>()(samplerCreateInfo.magFilter) + std::hash<uint32_t>()(samplerCreateInfo.minFilter) +
                       std::hash<uint32_t>()(samplerCreateInfo.addressModeU);
            }
        };

        struct SamplerKeyEqual
        {
            bool operator()(const VkSamplerCreateInfo& lhs, const VkSamplerCreateInfo& rhs) const
            {
                return lhs.minFilter == rhs.minFilter &&  //
                       lhs.magFilter == rhs.magFilter &&  //

                       lhs.addressModeU == rhs.addressModeU &&  //
                       lhs.addressModeV == rhs.addressModeV &&  //
                       lhs.addressModeW == rhs.addressModeW &&  //

                       lhs.anisotropyEnable == rhs.anisotropyEnable &&  //
                       lhs.maxAnisotropy == rhs.maxAnisotropy &&        //

                       lhs.borderColor == rhs.borderColor &&      //
                       lhs.compareEnable == rhs.compareEnable &&  //
                       lhs.compareOp == rhs.compareOp &&          //

                       lhs.mipmapMode == rhs.mipmapMode &&  //
                       lhs.mipLodBias == rhs.mipLodBias &&  //
                       lhs.minLod == rhs.minLod &&          //
                       lhs.maxLod == rhs.maxLod;
            }
        };

        std::unordered_map<VkSamplerCreateInfo, VkSampler, SamplerKeyHash, SamplerKeyEqual>
            Samplers;  // TODO: Make specific Sampler class with void* SamplerHandle and its Specification

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

    const Ref<Image>& GetFinalImageImpl() final override;
    std::vector<RendererOutput> GetRendererOutputImpl() final override;

    FORCEINLINE static VulkanRendererStorage& GetStorageData() { return s_Data; }

  private:
    VulkanContext& m_Context;

    inline static VulkanRendererStorage s_Data;

    void SetupSkybox();
    void DrawSkybox();
    void DestroySkybox();
};

}  // namespace Gauntlet
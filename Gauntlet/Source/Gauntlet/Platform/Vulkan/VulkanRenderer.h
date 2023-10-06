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
class Skybox;

class VulkanRenderer final : public Renderer
{
  private:
    // TODO: Move it
    struct GeometryData
    {
        Ref<Gauntlet::Material> Material;
        Ref<Gauntlet::VertexBuffer> VertexBuffer;
        Ref<Gauntlet::IndexBuffer> IndexBuffer;
        glm::mat4 Transform;
    };

    struct VulkanRendererStorage
    {
        // Viewports
        bool bFramebuffersNeedResize  = {false};
        glm::uvec2 NewFramebufferSize = {1280, 720};

        // Framebuffers && RenderPasses
        Ref<VulkanFramebuffer> ShadowMapFramebuffer = nullptr;
        Ref<VulkanFramebuffer> SetupFramebuffer     = nullptr;  // Clear pass
        Ref<VulkanFramebuffer> GeometryFramebuffer  = nullptr;
        Ref<VulkanFramebuffer> LightingFramebuffer  = nullptr;

        // SSAO
        Ref<VulkanFramebuffer> SSAOFramebuffer     = nullptr;
        Ref<VulkanPipeline> SSAOPipeline           = nullptr;
        Ref<VulkanTexture2D> SSAONoiseTexture      = nullptr;
        Ref<VulkanFramebuffer> SSAOBlurFramebuffer = nullptr;
        Ref<VulkanPipeline> SSAOBlurPipeline       = nullptr;

        // Deferred stuff
        DescriptorSet* LightingSet = nullptr;
        DescriptorSet* SSAOSet     = nullptr;
        DescriptorSet* SSAOBlurSet = nullptr;

        // Material
        VkDescriptorSetLayout GeometryDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout MeshDescriptorSetLayout     = VK_NULL_HANDLE;

        // Pipelines
        Ref<VulkanPipeline> ShadowMapPipeline   = nullptr;
        Ref<VulkanPipeline> ForwardPassPipeline = nullptr;
        Ref<VulkanPipeline> GeometryPipeline    = nullptr;
        Ref<VulkanPipeline> LightingPipeline    = nullptr;

        // Camera UB
        std::vector<VulkanAllocatedBuffer> UniformCameraDataBuffers;
        std::vector<void*> MappedUniformCameraDataBuffers;
        CameraDataBuffer MeshCameraDataBuffer;

        // Shadows UB
        std::vector<VulkanAllocatedBuffer> UniformShadowsBuffers;
        std::vector<void*> MappedUniformShadowsBuffers;
        ShadowsBuffer MeshShadowsBuffer;

        // SSAO UB
        std::vector<VulkanAllocatedBuffer> UniformSSAOBuffers;
        std::vector<void*> MappedUniformSSAOBuffers;
        SSAOBuffer SSAODataBuffer;

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

        // Sampler handling
        struct SamplerKeyHash
        {
            size_t operator()(const VkSamplerCreateInfo& samplerCreateInfo) const
            {
                return std::hash<size_t>()(samplerCreateInfo.magFilter) + std::hash<size_t>()(samplerCreateInfo.minFilter) +
                       std::hash<size_t>()(samplerCreateInfo.addressModeU);
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
        std::vector<VulkanAllocatedBuffer> UniformPhongModelBuffers;
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
                                 const glm::vec4& AmbientSpecularShininessCastShadows) final override;

    void BeginSceneImpl(const Camera& camera) final override;
    void EndSceneImpl() final override;
    void SubmitMeshImpl(const Ref<Mesh>& mesh, const glm::mat4& transform) final override;

    void BeginImpl() final override;
    void FlushImpl() final override;
    void ResizeFramebuffersImpl(uint32_t width, uint32_t height) final override;

    const Ref<Image>& GetFinalImageImpl() final override;
    std::vector<RendererOutput> GetRendererOutputImpl() final override;

    FORCEINLINE static VulkanRendererStorage& GetVulkanStorageData() { return s_Data; }

  private:
    VulkanContext& m_Context;

    inline static VulkanRendererStorage s_Data;

    void SetupSkybox();
    void DrawSkybox();
    void DestroySkybox();
};

}  // namespace Gauntlet
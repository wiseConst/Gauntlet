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
class Pipeline;
class VulkanShader;
class VulkanTexture2D;
class VulkanCommandBuffer;

class VulkanRenderer final : public Renderer
{
  private:
    struct VulkanRendererStorage
    {
        // Material
        VkDescriptorSetLayout GeometryDescriptorSetLayout = VK_NULL_HANDLE;

        // UI
        VkDescriptorSetLayout ImageDescriptorSetLayout = VK_NULL_HANDLE;

        // Misc
        VulkanCommandBuffer* CurrentCommandBuffer = nullptr;
        Weak<Pipeline> CurrentPipelineToBind;

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
    };

  public:
    VulkanRenderer();
    ~VulkanRenderer();

    void BeginImpl() final override;

    void BeginRenderPassImpl(const Ref<Framebuffer>& framebuffer, const glm::vec4& debugLabelColor = glm::vec4(1.0f)) final override;
    void EndRenderPassImpl(const Ref<Framebuffer>& framebuffer) final override;

    void SubmitMeshImpl(Ref<Pipeline>& pipeline, Ref<VertexBuffer>& vertexBuffer, Ref<IndexBuffer>& indexBuffer, Ref<Material>& material,
                        void* pushConstants = nullptr) final override;
    void SubmitFullscreenQuadImpl(Ref<Pipeline>& pipeline, void* pushConstants = nullptr) final override;

    void DrawQuadImpl(Ref<Pipeline>& pipeline, Ref<VertexBuffer>& vertexBuffer, Ref<IndexBuffer>& indexBuffer, const uint32_t indicesCount,
                      void* pushConstants = nullptr) final override;

    FORCEINLINE static VulkanRendererStorage& GetVulkanStorageData() { return s_Data; }

    void PostInit() final override;

  private:
    VulkanContext& m_Context;

    inline static VulkanRendererStorage s_Data;

    // TODO: In future this will be refactored since it assumes I'm not using offsets and multiple descriptor sets.
    void DrawIndexedInternal(Ref<Pipeline>& pipeline, const Ref<IndexBuffer>& indexBuffer, const Ref<VertexBuffer>& vertexBuffer,
                             void* pushConstants = nullptr, VkDescriptorSet* descriptorSets = nullptr, const uint32_t descriptorCount = 0);
};

}  // namespace Gauntlet
#pragma once

#include "Gauntlet/Renderer/Renderer.h"

#include <volk/volk.h>

#include "Gauntlet/Renderer/CoreRendererStructs.h"

namespace Gauntlet
{

class VulkanContext;

class VulkanRenderer final : public Renderer
{
  private:
    struct VulkanRendererStorage
    {
        // UI
        VkDescriptorSetLayout ImageDescriptorSetLayout = VK_NULL_HANDLE;

        // Misc
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

    // TODO: mb simply submit the whole particle system class?
    void SubmitParticleSystemImpl(const Ref<CommandBuffer>& commandBuffer, Ref<Pipeline>& pipeline, Ref<StorageBuffer>& ssbo,
                                  uint32_t particleCount, void* pushConstants = nullptr) final override;

    void BeginImpl() final override;
    void DispatchImpl(Ref<CommandBuffer>& commandBuffer, Ref<Pipeline>& pipeline, void* pushConstants = nullptr,
                      const uint32_t groupCountX = 1, const uint32_t groupCountY = 1, const uint32_t groupCountZ = 1) final override;

    void BeginRenderPassImpl(const Ref<CommandBuffer>& commandBuffer, const Ref<Framebuffer>& framebuffer,
                             const glm::vec4& debugLabelColor = glm::vec4(1.0f)) final override;
    void EndRenderPassImpl(const Ref<CommandBuffer>& commandBuffer, const Ref<Framebuffer>& framebuffer) final override;

    void SubmitMeshImpl(Ref<Pipeline>& pipeline, Ref<VertexBuffer>& vertexBuffer, Ref<IndexBuffer>& indexBuffer, Ref<Material>& material,
                        void* pushConstants = nullptr) final override;
    void SubmitFullscreenQuadImpl(Ref<Pipeline>& pipeline, void* pushConstants = nullptr) final override;

    void DrawQuadImpl(Ref<Pipeline>& pipeline, Ref<VertexBuffer>& vertexBuffer, Ref<IndexBuffer>& indexBuffer, const uint32_t indicesCount,
                      void* pushConstants = nullptr) final override;

    FORCEINLINE static VulkanRendererStorage& GetVulkanStorageData() { return s_Data; }

  private:
    VulkanContext& m_Context;

    inline static VulkanRendererStorage s_Data;

    // TODO: In future this will be refactored since it assumes I'm not using offsets and multiple descriptor sets.
    void DrawIndexedInternal(Ref<Pipeline>& pipeline, const Ref<IndexBuffer>& indexBuffer, const Ref<VertexBuffer>& vertexBuffer,
                             void* pushConstants = nullptr, VkDescriptorSet* descriptorSets = nullptr, const uint32_t descriptorCount = 0);
};

}  // namespace Gauntlet
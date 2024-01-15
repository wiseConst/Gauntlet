#pragma once

#include "Gauntlet/Renderer/Renderer.h"

#include <volk/volk.h>

#include "Gauntlet/Renderer/CoreRendererTypes.h"

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
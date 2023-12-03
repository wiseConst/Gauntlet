#pragma once

#include "Gauntlet/Renderer/Pipeline.h"

#include <volk/volk.h>

namespace Gauntlet
{
class VulkanShader;
class VulkanFramebuffer;

class VulkanPipeline final : public Pipeline
{
  public:
    VulkanPipeline(const PipelineSpecification& pipelineSpecification);
    ~VulkanPipeline() = default;

    void Destroy() final override;

    FORCEINLINE auto& Get() const { return m_Handle; }
    FORCEINLINE auto& GetLayout() const { return m_Layout; }
    FORCEINLINE PipelineSpecification& GetSpecification() final override { return m_Specification; }

    const VkShaderStageFlags GetPushConstantsShaderStageFlags(const uint32_t Index = 0) const;
    const uint32_t GetPushConstantsSize(const uint32_t Index = 0) const;

  private:
    PipelineSpecification m_Specification;
    VkPipeline m_Handle       = VK_NULL_HANDLE;
    VkPipelineCache m_Cache   = VK_NULL_HANDLE;
    VkPipelineLayout m_Layout = VK_NULL_HANDLE;

    void Invalidate();  // No longer invalidating pipeline in runtime, only creating it once.

    void CreateLayout();
    void Create();
    void CreateOrRetrieveAndValidatePipelineCache();
    void SavePipelineCache();
};

}  // namespace Gauntlet
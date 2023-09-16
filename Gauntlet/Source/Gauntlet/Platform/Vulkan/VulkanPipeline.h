#pragma once

#include "Gauntlet/Core/Core.h"
#include "VulkanUtility.h"
#include <volk/volk.h>

namespace Gauntlet
{
class VulkanShader;
class VulkanFramebuffer;

struct PipelineSpecification
{
  public:
    PipelineSpecification()  = default;
    ~PipelineSpecification() = default;

    std::string Name = "None";

    Ref<VulkanShader> Shader                 = nullptr;
    Ref<VulkanFramebuffer> TargetFramebuffer = nullptr;

    VkBool32 PrimitiveRestartEnable      = VK_FALSE;
    EPrimitiveTopology PrimitiveTopology = EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    ECullMode CullMode       = ECullMode::CULL_MODE_NONE;
    float LineWidth          = 1.0f;
    EPolygonMode PolygonMode = EPolygonMode::POLYGON_MODE_FILL;
    EFrontFace FrontFace     = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;

    VkBool32 bDepthTest          = VK_FALSE;  // If we should do any z-culling at all
    VkBool32 bDepthWrite         = VK_FALSE;  // Allows the depth to be written.
    VkBool32 bDynamicPolygonMode = VK_FALSE;
    VkBool32 bBlendEnable        = VK_FALSE;
    VkCompareOp DepthCompareOp   = VK_COMPARE_OP_ALWAYS;
};

class VulkanPipeline final
{
  public:
    VulkanPipeline(const PipelineSpecification& pipelineSpecification);
    VulkanPipeline()  = delete;
    ~VulkanPipeline() = default;

    void Destroy();

    FORCEINLINE auto& Get() { return m_Pipeline; }
    FORCEINLINE auto& GetLayout() { return m_PipelineLayout; }

    const VkShaderStageFlags& GetPushConstantsShaderStageFlags(const uint32_t Index = 0);
    uint32_t GetPushConstantsSize(const uint32_t Index = 0);

    FORCEINLINE auto& GetSpecification() { return m_Specification; }

  private:
    PipelineSpecification m_Specification;
    VkPipeline m_Pipeline             = VK_NULL_HANDLE;
    VkPipelineCache m_Cache           = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

    void Invalidate();  // No longer invalidating pipeline in runtime, only creating it once.

    void CreatePipelineLayout();
    void CreatePipeline();
};

}  // namespace Gauntlet
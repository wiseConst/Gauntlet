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
    PipelineSpecification()
    {
        ShaderStages.resize(0);

        DescriptorSetLayouts.resize(0);
        PushConstantRanges.resize(0);

        ShaderAttributeDescriptions.resize(0);
        ShaderBindingDescriptions.resize(0);

        LineWidth   = 1.0f;
        FrontFace   = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        PolygonMode = EPolygonMode::POLYGON_MODE_FILL;
        CullMode    = ECullMode::CULL_MODE_NONE;

        TargetFramebuffer      = nullptr;
        PrimitiveRestartEnable = VK_FALSE;
        PrimitiveTopology      = EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // No depth testing by default.
        bDepthTest     = VK_FALSE;
        bDepthWrite    = VK_FALSE;
        DepthCompareOp = VK_COMPARE_OP_ALWAYS;
    }
    ~PipelineSpecification() = default;

    std::string Name = "None";

    std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
    std::vector<VkPushConstantRange> PushConstantRanges;

    VkBool32 PrimitiveRestartEnable;
    EPrimitiveTopology PrimitiveTopology;

    struct ShaderStage
    {
        Ref<VulkanShader> Shader;
        EShaderStage Stage;
    };
    std::vector<ShaderStage> ShaderStages;

    std::vector<VkVertexInputAttributeDescription>
        ShaderAttributeDescriptions;                                         // Per-vertex attribute data(all 'in' attributes description)
    std::vector<VkVertexInputBindingDescription> ShaderBindingDescriptions;  // Description of per-vertex data(it's stride)
    Ref<VulkanFramebuffer> TargetFramebuffer;

    ECullMode CullMode;
    float LineWidth;  // By default should be 1.0f
    EPolygonMode PolygonMode;
    EFrontFace FrontFace;

    VkBool32 bDepthTest;   // If we should do any z-culling at all
    VkBool32 bDepthWrite;  // Allows the depth to be written.
    VkCompareOp DepthCompareOp;
};

class VulkanPipeline final
{
  public:
    VulkanPipeline(const PipelineSpecification& pipelineSpecification);
    VulkanPipeline()  = delete;
    ~VulkanPipeline() = default;

    void Invalidate();
    void Destroy();

    FORCEINLINE auto& Get() { return m_Pipeline; }
    FORCEINLINE auto& GetLayout() { return m_PipelineLayout; }

    FORCEINLINE const auto& GetPushConstantsShaderStageFlags(const uint32_t Index = 0)
    {
        return m_Specification.PushConstantRanges[Index].stageFlags;
    }

    FORCEINLINE const auto& GetPushConstantsSize(const uint32_t Index = 0) { return m_Specification.PushConstantRanges[Index].size; }
    FORCEINLINE auto& GetSpecification() { return m_Specification; }

  private:
    PipelineSpecification m_Specification;
    VkPipeline m_Pipeline             = VK_NULL_HANDLE;
    VkPipelineCache m_Cache           = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

    void CreatePipelineLayout();
    void CreatePipeline();
};

namespace PipelineUtils
{
VkPrimitiveTopology GauntletPrimitiveTopologyToVulkan(EPrimitiveTopology primitiveTopology);

VkShaderStageFlagBits GauntletShaderStageToVulkan(EShaderStage shaderStage);

VkCullModeFlagBits GauntletCullModeToVulkan(ECullMode cullMode);

VkPolygonMode GauntletPolygonModeToVulkan(EPolygonMode polygonMode);

VkFrontFace GauntletFrontFaceToVulkan(EFrontFace frontFace);

}  // namespace PipelineUtils

}  // namespace Gauntlet
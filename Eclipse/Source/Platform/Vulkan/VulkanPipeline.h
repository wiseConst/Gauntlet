#pragma once

#include "Eclipse/Core/Core.h"
#include <volk/volk.h>

namespace Eclipse
{
class VulkanShader;

enum class EPrimitiveTopology : uint8_t
{
    PRIMITIVE_TOPOLOGY_POINT_LIST = 0,
    PRIMITIVE_TOPOLOGY_LINE_LIST,
    PRIMITIVE_TOPOLOGY_LINE_STRIP,
    PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
    PRIMITIVE_TOPOLOGY_TRIANGLE_FAN
};

// TODO: Mb it should be in Shader class
enum class EShaderStage : uint8_t
{
    SHADER_STAGE_VERTEX = 0,
    SHADER_STAGE_GEOMETRY,
    SHADER_STAGE_FRAGMENT,
    SHADER_STAGE_COMPUTE
};

enum class ECullMode : uint8_t
{
    CULL_MODE_NONE = 0,
    CULL_MODE_FRONT,
    CULL_MODE_BACK,
    CULL_MODE_FRONT_AND_BACK
};

enum class EPolygonMode : uint8_t
{
    POLYGON_MODE_FILL = 0,
    POLYGON_MODE_LINE,
    POLYGON_MODE_POINT,
    POLYGON_MODE_FILL_RECTANGLE_NV
};

enum class EFrontFace : uint8_t
{
    FRONT_FACE_COUNTER_CLOCKWISE = 0,
    FRONT_FACE_CLOCKWISE
};

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

        RenderPass             = VK_NULL_HANDLE;
        PrimitiveRestartEnable = VK_FALSE;
        PrimitiveTopology      = EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // No depth testing by default.
        bDepthTest     = VK_FALSE;
        bDepthWrite    = VK_FALSE;
        DepthCompareOp = VK_COMPARE_OP_ALWAYS;
    }
    ~PipelineSpecification() = default;

    std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
    std::vector<VkPushConstantRange> PushConstantRanges;

    VkBool32 PrimitiveRestartEnable;
    EPrimitiveTopology PrimitiveTopology;

    struct ShaderStage
    {
        // std::string_view EntryPointName = "main";
        Ref<VulkanShader> Shader;
        EShaderStage Stage;
    };
    std::vector<ShaderStage> ShaderStages;

    // VkVertexInputAttributeDescription defines vertex shader attribute. For instance, layout(location = 0) in vec3 InPosition
    // etc..(location in this case) and it's offset in buffer
    std::vector<VkVertexInputAttributeDescription> ShaderAttributeDescriptions;

    // VkVertexInputBindingDescription defines the vertex buffers that act as input.
    std::vector<VkVertexInputBindingDescription> ShaderBindingDescriptions;

    VkRenderPass RenderPass;

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
    VulkanPipeline(const PipelineSpecification& InPipelineSpecification);
    VulkanPipeline()  = delete;
    ~VulkanPipeline() = default;

    void Create();
    void Destroy();

    FORCEINLINE const auto& Get() const { return m_Pipeline; }
    FORCEINLINE auto& Get() { return m_Pipeline; }

    FORCEINLINE const auto& GetLayout() const { return m_PipelineLayout; }
    FORCEINLINE auto& GetLayout() { return m_PipelineLayout; }

    // TEST PURPOSES
    FORCEINLINE const auto& GetPushConstantsShaderStageFlags(const uint32_t Index = 0) const
    {
        return m_PipelineSpecification.PushConstantRanges[Index].stageFlags;
    }

    FORCEINLINE auto& GetPushConstantsShaderStageFlags(const uint32_t Index = 0)
    {
        return m_PipelineSpecification.PushConstantRanges[Index].stageFlags;
    }

    FORCEINLINE const auto& GetPushConstantsSize(const uint32_t Index = 0) const
    {
        return m_PipelineSpecification.PushConstantRanges[Index].size;
    }

    FORCEINLINE auto& GetPushConstantsSize(const uint32_t Index = 0) { return m_PipelineSpecification.PushConstantRanges[Index].size; }

    FORCEINLINE void SetRenderPass(const VkRenderPass& InRenderPass) { m_PipelineSpecification.RenderPass = InRenderPass; }

  private:
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    PipelineSpecification m_PipelineSpecification;

    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

    void CreatePipelineLayout();
    void CreatePipeline();
};

}  // namespace Eclipse
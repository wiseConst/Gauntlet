#include "GauntletPCH.h"
#include "VulkanPipeline.h"

#include "VulkanUtility.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanShader.h"
#include "VulkanSwapchain.h"

#include "Gauntlet/Core/Application.h"

namespace Gauntlet
{

static VkPrimitiveTopology GauntletPrimitiveTopologyToVulkan(EPrimitiveTopology primitiveTopology)
{
    switch (primitiveTopology)
    {
        case EPrimitiveTopology::PRIMITIVE_TOPOLOGY_POINT_LIST: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case EPrimitiveTopology::PRIMITIVE_TOPOLOGY_LINE_LIST: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case EPrimitiveTopology::PRIMITIVE_TOPOLOGY_LINE_STRIP: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_FAN: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    }

    GNT_ASSERT(false, "Unknown primitve topology flag!");
    return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
}

static VkShaderStageFlagBits GauntletShaderStageToVulkan(EShaderStage shaderStage)
{
    switch (shaderStage)
    {
        case EShaderStage::SHADER_STAGE_VERTEX: return VK_SHADER_STAGE_VERTEX_BIT;
        case EShaderStage::SHADER_STAGE_GEOMETRY: return VK_SHADER_STAGE_GEOMETRY_BIT;
        case EShaderStage::SHADER_STAGE_FRAGMENT: return VK_SHADER_STAGE_FRAGMENT_BIT;
        case EShaderStage::SHADER_STAGE_COMPUTE: return VK_SHADER_STAGE_COMPUTE_BIT;
    }

    GNT_ASSERT(false, "Unknown shader stage flag!");
    return VK_SHADER_STAGE_VERTEX_BIT;
}

static VkCullModeFlagBits GauntletCullModeToVulkan(ECullMode cullMode)
{
    switch (cullMode)
    {
        case ECullMode::CULL_MODE_NONE: return VK_CULL_MODE_NONE;
        case ECullMode::CULL_MODE_FRONT: return VK_CULL_MODE_FRONT_BIT;
        case ECullMode::CULL_MODE_BACK: return VK_CULL_MODE_BACK_BIT;
        case ECullMode::CULL_MODE_FRONT_AND_BACK: return VK_CULL_MODE_FRONT_AND_BACK;
    }

    GNT_ASSERT(false, "Unknown cull mode flag!");
    return VK_CULL_MODE_NONE;
}

static VkPolygonMode GauntletPolygonModeToVulkan(EPolygonMode polygonMode)
{
    switch (polygonMode)
    {
        case EPolygonMode::POLYGON_MODE_FILL: return VK_POLYGON_MODE_FILL;
        case EPolygonMode::POLYGON_MODE_LINE: return VK_POLYGON_MODE_LINE;
        case EPolygonMode::POLYGON_MODE_POINT: return VK_POLYGON_MODE_POINT;
        case EPolygonMode::POLYGON_MODE_FILL_RECTANGLE_NV: return VK_POLYGON_MODE_FILL_RECTANGLE_NV;
    }

    GNT_ASSERT(false, "Unknown polygon mode flag!");
    return VK_POLYGON_MODE_FILL;
}

static VkFrontFace GauntletFrontFaceToVulkan(EFrontFace frontFace)
{
    switch (frontFace)
    {
        case EFrontFace::FRONT_FACE_CLOCKWISE: return VK_FRONT_FACE_CLOCKWISE;
        case EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }

    GNT_ASSERT(false, "Unknown front face flag!");
    return VK_FRONT_FACE_CLOCKWISE;
}

VulkanPipeline::VulkanPipeline(const PipelineSpecification& pipelineSpecification) : m_Specification(pipelineSpecification)
{
    Invalidate();
}

void VulkanPipeline::Invalidate()
{
    if (m_Pipeline) Destroy();

    CreatePipelineLayout();

    const float PipelineCreationBegin = Application::Get().GetTimeNow();
    CreatePipeline();
    const float PipelineCreationEnd = Application::Get().GetTimeNow();
    LOG_INFO("Took %0.3f ms to create %s!", (PipelineCreationEnd - PipelineCreationBegin) * 1000.0f, m_Specification.Name.data());
}

void VulkanPipeline::CreatePipelineLayout()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    /*
     * Pipeline layouts contains the information about shader inputs of a given pipeline.
     * It’s here where you would configure your push constants and descriptor sets.
     */
    VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    PipelineLayoutCreateInfo.setLayoutCount             = static_cast<uint32_t>(m_Specification.DescriptorSetLayouts.size());
    PipelineLayoutCreateInfo.pSetLayouts                = m_Specification.DescriptorSetLayouts.data();
    PipelineLayoutCreateInfo.pushConstantRangeCount     = static_cast<uint32_t>(m_Specification.PushConstantRanges.size());
    PipelineLayoutCreateInfo.pPushConstantRanges        = m_Specification.PushConstantRanges.data();

    VK_CHECK(vkCreatePipelineLayout(Context.GetDevice()->GetLogicalDevice(), &PipelineLayoutCreateInfo, nullptr, &m_PipelineLayout),
             "Failed to create pipeline layout!");
}

void VulkanPipeline::CreatePipeline()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");
    GNT_ASSERT(Context.GetSwapchain()->IsValid(), "Vulkan swapchain is not valid!");

    // Creating pipeline cache
    {
        std::vector<uint8_t> CacheData =
            Utility::LoadPipelineCacheFromDisk(std::string(ASSETS_PATH) + "Cached/Pipelines/" + m_Specification.Name + ".cache");

        VkPipelineCacheCreateInfo PipelineCacheCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
        PipelineCacheCreateInfo.initialDataSize           = CacheData.size();
        PipelineCacheCreateInfo.pInitialData              = CacheData.data();

        VK_CHECK(vkCreatePipelineCache(Context.GetDevice()->GetLogicalDevice(), &PipelineCacheCreateInfo, nullptr, &m_Cache),
                 "Failed to create pipeline cache!");
    }

    // Contains the configuration for what kind of topology will be drawn.
    VkPipelineInputAssemblyStateCreateInfo InputAssemblyState = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    InputAssemblyState.primitiveRestartEnable                 = m_Specification.PrimitiveRestartEnable;
    InputAssemblyState.topology                               = GauntletPrimitiveTopologyToVulkan(m_Specification.PrimitiveTopology);

    std::vector<VkPipelineShaderStageCreateInfo> ShaderStages(m_Specification.ShaderStages.size());
    for (size_t i = 0; i < m_Specification.ShaderStages.size(); ++i)
    {
        ShaderStages[i].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStages[i].pName  = "main";
        ShaderStages[i].module = m_Specification.ShaderStages[i].Shader->GetModule();
        ShaderStages[i].stage  = GauntletShaderStageToVulkan(m_Specification.ShaderStages[i].Stage);
    }

    // Contains the information for vertex buffers and vertex formats.
    // This is equivalent to the VAO configuration in OpenGL.
    VkPipelineVertexInputStateCreateInfo VertexInputState = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VertexInputState.vertexAttributeDescriptionCount      = static_cast<uint32_t>(m_Specification.ShaderAttributeDescriptions.size());
    VertexInputState.pVertexAttributeDescriptions         = m_Specification.ShaderAttributeDescriptions.data();
    VertexInputState.vertexBindingDescriptionCount        = static_cast<uint32_t>(m_Specification.ShaderBindingDescriptions.size());
    VertexInputState.pVertexBindingDescriptions           = m_Specification.ShaderBindingDescriptions.data();

    // Breakes primitives into small ones
    VkPipelineTessellationStateCreateInfo TessellationState = {VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO};

    // Configuration for the fixed-function rasterization. In here is where we enable or disable backface culling, and set line width or
    // wireframe drawing.
    VkPipelineRasterizationStateCreateInfo RasterizationState = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    RasterizationState.cullMode                               = GauntletCullModeToVulkan(m_Specification.CullMode);
    RasterizationState.lineWidth                              = m_Specification.LineWidth;
    RasterizationState.polygonMode                            = GauntletPolygonModeToVulkan(m_Specification.PolygonMode);
    RasterizationState.frontFace                              = GauntletFrontFaceToVulkan(m_Specification.FrontFace);

    /*
     *  If rasterizerDiscardEnable is enabled, primitives (triangles in our case) are discarded
     *  before even making it to the rasterization stage which means the triangles would never get drawn to the screen.
     *  You might enable this, for example, if you’re only interested in the side effects of the vertex processing stages,
     *  such as writing to a buffer which you later read from. But in our case we’re interested in drawing the triangle,
     *  so we leave it disabled.
     */
    RasterizationState.rasterizerDiscardEnable = VK_FALSE;

    // TODO: Make it configurable?
    RasterizationState.depthClampEnable        = VK_FALSE;
    RasterizationState.depthBiasEnable         = VK_FALSE;
    RasterizationState.depthBiasConstantFactor = 0.0f;
    RasterizationState.depthBiasClamp          = 0.0f;
    RasterizationState.depthBiasSlopeFactor    = 0.0f;

    // TODO: Make it configurable?
    VkPipelineMultisampleStateCreateInfo MultisampleState = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    MultisampleState.sampleShadingEnable                  = VK_FALSE;
    MultisampleState.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;
    MultisampleState.minSampleShading                     = 1.0f;
    MultisampleState.alphaToCoverageEnable                = VK_FALSE;
    MultisampleState.alphaToOneEnable                     = VK_FALSE;

    // TODO: Make it configurable? Now by default it has alpha-blending
    VkPipelineColorBlendAttachmentState ColorBlendAttachment = {};
    ColorBlendAttachment.blendEnable                         = TRUE;
    ColorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    // Color.rgb = (src.a * src.rgb) + ((1-src.a) * dst.rgb)
    ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    ColorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    // Optional
    ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    ColorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

    // Controls how this pipeline blends into a given attachment.
    /*
     * Setup dummy color blending. We aren't using transparent objects yet
     * the blending is just "no blend", but we do write to the color attachment
     */
    VkPipelineColorBlendStateCreateInfo ColorBlendState = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    ColorBlendState.attachmentCount                     = 1;
    ColorBlendState.pAttachments                        = &ColorBlendAttachment;
    ColorBlendState.logicOpEnable                       = VK_FALSE;
    ColorBlendState.logicOp                             = VK_LOGIC_OP_COPY;

    // Unlike OpenGL([-1, 1]), Vulkan has depth range [0, 1]
    // Also flip up the viewport and set frontface to ccw
    VkViewport Viewport = {};
    Viewport.height     = -static_cast<float>(Context.GetSwapchain()->GetImageExtent().height);
    Viewport.width      = static_cast<float>(Context.GetSwapchain()->GetImageExtent().width);
    Viewport.minDepth   = 0.0f;
    Viewport.maxDepth   = 1.0f;
    Viewport.x          = 0.0f;
    Viewport.y          = static_cast<float>(Context.GetSwapchain()->GetImageExtent().height);

    VkRect2D Scissor = {};
    Scissor.offset   = {0, 0};
    Scissor.extent   = Context.GetSwapchain()->GetImageExtent();

    // TODO: Make it configurable?
    VkPipelineViewportStateCreateInfo ViewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    ViewportState.viewportCount                     = 1;
    ViewportState.pViewports                        = &Viewport;
    ViewportState.scissorCount                      = 1;
    ViewportState.pScissors                         = &Scissor;

    VkPipelineDepthStencilStateCreateInfo DepthStencilState = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    DepthStencilState.depthTestEnable                       = m_Specification.bDepthTest ? VK_TRUE : VK_FALSE;
    DepthStencilState.depthWriteEnable                      = m_Specification.bDepthWrite ? VK_TRUE : VK_FALSE;
    DepthStencilState.depthCompareOp        = m_Specification.bDepthTest ? m_Specification.DepthCompareOp : VK_COMPARE_OP_ALWAYS;
    DepthStencilState.depthBoundsTestEnable = VK_FALSE;
    DepthStencilState.stencilTestEnable     = VK_FALSE;

    /*
     * MIN && MAX depth bounds lets us cap the depth test.
     * If the depth is outside of bounds, the pixel will be skipped.
     */
    DepthStencilState.minDepthBounds = 0.0f;
    DepthStencilState.maxDepthBounds = 1.0f;

    std::vector<VkDynamicState> DynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    // TODO: Make it configurable?
    VkPipelineDynamicStateCreateInfo DynamicState = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    DynamicState.dynamicStateCount                = static_cast<uint32_t>(DynamicStates.size());
    DynamicState.pDynamicStates                   = DynamicStates.data();

    VkGraphicsPipelineCreateInfo PipelineCreateInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    PipelineCreateInfo.layout                       = m_PipelineLayout;
    PipelineCreateInfo.renderPass                   = m_Specification.TargetFramebuffer->GetRenderPass();
    PipelineCreateInfo.pInputAssemblyState          = &InputAssemblyState;
    PipelineCreateInfo.stageCount                   = static_cast<uint32_t>(ShaderStages.size());
    PipelineCreateInfo.pStages                      = ShaderStages.data();
    PipelineCreateInfo.pVertexInputState            = &VertexInputState;
    PipelineCreateInfo.pRasterizationState          = &RasterizationState;
    PipelineCreateInfo.pTessellationState           = &TessellationState;
    PipelineCreateInfo.pViewportState               = &ViewportState;
    PipelineCreateInfo.pMultisampleState            = &MultisampleState;
    PipelineCreateInfo.pDepthStencilState           = &DepthStencilState;
    PipelineCreateInfo.pColorBlendState             = &ColorBlendState;
    PipelineCreateInfo.pDynamicState                = &DynamicState;

    VK_CHECK(
        vkCreateGraphicsPipelines(Context.GetDevice()->GetLogicalDevice(), m_Cache, 1, &PipelineCreateInfo, VK_NULL_HANDLE, &m_Pipeline),
        "Failed to create pipeline!");
}

void VulkanPipeline::Destroy()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");
    auto& LogicalDevice = Context.GetDevice()->GetLogicalDevice();

    GNT_ASSERT(Utility::DropPipelineCacheToDisk(
                   LogicalDevice, m_Cache, std::string(ASSETS_PATH) + "Cached/Pipelines/" + m_Specification.Name + ".cache") == VK_TRUE,
               "Failed to save pipeline cache to disk!");

    vkDestroyPipelineCache(LogicalDevice, m_Cache, nullptr);

    vkDestroyPipelineLayout(LogicalDevice, m_PipelineLayout, nullptr);
    vkDestroyPipeline(LogicalDevice, m_Pipeline, nullptr);
}

}  // namespace Gauntlet
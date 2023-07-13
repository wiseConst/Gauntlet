#include "EclipsePCH.h"
#include "VulkanPipeline.h"

#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanShader.h"
#include "VulkanSwapchain.h"

namespace Eclipse
{
static VkPrimitiveTopology EclipsePrimitiveTopologyToVulkan(EPrimitiveTopology InPrimitiveTopology)
{
    switch (InPrimitiveTopology)
    {
        case EPrimitiveTopology::PRIMITIVE_TOPOLOGY_POINT_LIST: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case EPrimitiveTopology::PRIMITIVE_TOPOLOGY_LINE_LIST: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case EPrimitiveTopology::PRIMITIVE_TOPOLOGY_LINE_STRIP: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_FAN: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    }

    ELS_ASSERT(false, "Unknown primitve topology flag!");
    return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
}

static VkShaderStageFlagBits EclipseShaderStageToVulkan(EShaderStage InShaderStage)
{
    switch (InShaderStage)
    {
        case EShaderStage::SHADER_STAGE_VERTEX: return VK_SHADER_STAGE_VERTEX_BIT;
        case EShaderStage::SHADER_STAGE_GEOMETRY: return VK_SHADER_STAGE_GEOMETRY_BIT;
        case EShaderStage::SHADER_STAGE_FRAGMENT: return VK_SHADER_STAGE_FRAGMENT_BIT;
        case EShaderStage::SHADER_STAGE_COMPUTE: return VK_SHADER_STAGE_COMPUTE_BIT;
    }

    ELS_ASSERT(false, "Unknown shader stage flag!");
    return VK_SHADER_STAGE_VERTEX_BIT;
}

static VkCullModeFlagBits EclipseCullModeToVulkan(ECullMode InCullMode)
{
    switch (InCullMode)
    {
        case ECullMode::CULL_MODE_NONE: return VK_CULL_MODE_NONE;
        case ECullMode::CULL_MODE_FRONT: return VK_CULL_MODE_FRONT_BIT;
        case ECullMode::CULL_MODE_BACK: return VK_CULL_MODE_BACK_BIT;
        case ECullMode::CULL_MODE_FRONT_AND_BACK: return VK_CULL_MODE_FRONT_AND_BACK;
    }

    ELS_ASSERT(false, "Unknown cull mode flag!");
    return VK_CULL_MODE_NONE;
}

static VkPolygonMode EclipsePolygonModeToVulkan(EPolygonMode InPolygonMode)
{
    switch (InPolygonMode)
    {
        case EPolygonMode::POLYGON_MODE_FILL: return VK_POLYGON_MODE_FILL;
        case EPolygonMode::POLYGON_MODE_LINE: return VK_POLYGON_MODE_LINE;
        case EPolygonMode::POLYGON_MODE_POINT: return VK_POLYGON_MODE_POINT;
        case EPolygonMode::POLYGON_MODE_FILL_RECTANGLE_NV: return VK_POLYGON_MODE_FILL_RECTANGLE_NV;
    }

    ELS_ASSERT(false, "Unknown polygon mode flag!");
    return VK_POLYGON_MODE_FILL;
}

static VkFrontFace EclipseFrontFaceToVulkan(EFrontFace InFrontFace)
{
    switch (InFrontFace)
    {
        case EFrontFace::FRONT_FACE_CLOCKWISE: return VK_FRONT_FACE_CLOCKWISE;
        case EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }

    ELS_ASSERT(false, "Unknown front face flag!");
    return VK_FRONT_FACE_CLOCKWISE;
}

VulkanPipeline::VulkanPipeline(const PipelineSpecification& InPipelineSpecification) : m_PipelineSpecification(InPipelineSpecification)
{
    Create();
}

void VulkanPipeline::Create()
{
    CreatePipelineLayout();
    CreatePipeline();
}

void VulkanPipeline::CreatePipelineLayout()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    /*
     * Pipeline layouts contains the information about shader inputs of a given pipeline.
     * It’s here where you would configure your push constants and descriptor sets.
     */
    VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = {};
    PipelineLayoutCreateInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    PipelineLayoutCreateInfo.setLayoutCount             = static_cast<uint32_t>(m_PipelineSpecification.DescriptorSetLayouts.size());
    PipelineLayoutCreateInfo.pSetLayouts                = m_PipelineSpecification.DescriptorSetLayouts.data();
    PipelineLayoutCreateInfo.pushConstantRangeCount     = static_cast<uint32_t>(m_PipelineSpecification.PushConstantRanges.size());
    PipelineLayoutCreateInfo.pPushConstantRanges        = m_PipelineSpecification.PushConstantRanges.data();

    VK_CHECK(vkCreatePipelineLayout(Context.GetDevice()->GetLogicalDevice(), &PipelineLayoutCreateInfo, nullptr, &m_PipelineLayout),
             "Failed to create pipeline layout!");
}

void VulkanPipeline::CreatePipeline()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");
    ELS_ASSERT(Context.GetSwapchain()->IsValid(), "Vulkan swapchain is not valid!");

    // Contains the configuration for what kind of topology will be drawn.
    VkPipelineInputAssemblyStateCreateInfo InputAssemblyState = {};
    InputAssemblyState.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyState.primitiveRestartEnable                 = m_PipelineSpecification.PrimitiveRestartEnable;
    InputAssemblyState.topology                               = EclipsePrimitiveTopologyToVulkan(m_PipelineSpecification.PrimitiveTopology);

    std::vector<VkPipelineShaderStageCreateInfo> ShaderStages(m_PipelineSpecification.ShaderStages.size());
    for (size_t i = 0; i < m_PipelineSpecification.ShaderStages.size(); ++i)
    {
        ShaderStages[i].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStages[i].pName  = "main";  // m_PipelineSpecification.ShaderStages[i].EntryPointName.data();
        ShaderStages[i].module = m_PipelineSpecification.ShaderStages[i].Shader->GetModule();
        ShaderStages[i].stage  = EclipseShaderStageToVulkan(m_PipelineSpecification.ShaderStages[i].Stage);
    }

    // Contains the information for vertex buffers and vertex formats.
    // This is equivalent to the VAO configuration in OpenGL.
    VkPipelineVertexInputStateCreateInfo VertexInputState = {};
    VertexInputState.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_PipelineSpecification.ShaderAttributeDescriptions.size());
    VertexInputState.pVertexAttributeDescriptions    = m_PipelineSpecification.ShaderAttributeDescriptions.data();
    VertexInputState.vertexBindingDescriptionCount   = static_cast<uint32_t>(m_PipelineSpecification.ShaderBindingDescriptions.size());
    VertexInputState.pVertexBindingDescriptions      = m_PipelineSpecification.ShaderBindingDescriptions.data();

    // Breakes primitives into small ones
    VkPipelineTessellationStateCreateInfo TessellationState = {};
    TessellationState.sType                                 = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;

    // Configuration for the fixed-function rasterization. In here is where we enable or disable backface culling, and set line width or
    // wireframe drawing.
    VkPipelineRasterizationStateCreateInfo RasterizationState = {};
    RasterizationState.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    RasterizationState.cullMode                               = EclipseCullModeToVulkan(m_PipelineSpecification.CullMode);
    RasterizationState.lineWidth                              = m_PipelineSpecification.LineWidth;
    RasterizationState.polygonMode                            = EclipsePolygonModeToVulkan(m_PipelineSpecification.PolygonMode);
    RasterizationState.frontFace                              = EclipseFrontFaceToVulkan(m_PipelineSpecification.FrontFace);

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
    VkPipelineMultisampleStateCreateInfo MultisampleState = {};
    MultisampleState.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    MultisampleState.sampleShadingEnable                  = VK_FALSE;
    MultisampleState.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;
    MultisampleState.minSampleShading                     = 1.0f;
    MultisampleState.pSampleMask                          = nullptr;
    MultisampleState.alphaToCoverageEnable                = VK_FALSE;
    MultisampleState.alphaToOneEnable                     = VK_FALSE;

    // TODO: Make it configurable?
    // We are rendering to only 1 attachment, so we will just need one of them,
    // and defaulted to “not blend” and just override. In here it’s possible
    // to make objects that will blend with the image
    VkPipelineColorBlendAttachmentState ColorBlendAttachment = {};
    ColorBlendAttachment.blendEnable                         = VK_FALSE;
    ColorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    // Controls how this pipeline blends into a given attachment.
    /*
     * Setup dummy color blending. We aren't using transparent objects yet
     * the blending is just "no blend", but we do write to the color attachment
     */
    VkPipelineColorBlendStateCreateInfo ColorBlendState = {};
    ColorBlendState.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColorBlendState.attachmentCount                     = 1;
    ColorBlendState.pAttachments                        = &ColorBlendAttachment;
    ColorBlendState.logicOpEnable                       = VK_FALSE;
    ColorBlendState.logicOp                             = VK_LOGIC_OP_COPY;

    VkViewport Viewport = {};
    Viewport.height     = -static_cast<float>(Context.GetSwapchain()->GetImageExtent().height);
    Viewport.width      = static_cast<float>(Context.GetSwapchain()->GetImageExtent().width);
    // Unlike OpenGL([-1, 1]), Vulkan has depth range [0, 1]
    Viewport.minDepth = 0.0f;
    Viewport.maxDepth = 1.0f;
    Viewport.x        = 0.0f;
    Viewport.y        = static_cast<float>(Context.GetSwapchain()->GetImageExtent().height);

    VkRect2D Scissor = {};
    Scissor.offset   = {0, 0};
    Scissor.extent   = Context.GetSwapchain()->GetImageExtent();

    // TODO: Make it configurable?
    VkPipelineViewportStateCreateInfo ViewportState = {};
    ViewportState.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportState.viewportCount                     = 1;
    ViewportState.pViewports                        = &Viewport;
    ViewportState.scissorCount                      = 1;
    ViewportState.pScissors                         = &Scissor;

    VkPipelineDepthStencilStateCreateInfo DepthStencilState = {};
    DepthStencilState.sType                                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilState.depthTestEnable                       = m_PipelineSpecification.bDepthTest ? VK_TRUE : VK_FALSE;
    DepthStencilState.depthWriteEnable                      = m_PipelineSpecification.bDepthWrite ? VK_TRUE : VK_FALSE;
    DepthStencilState.depthCompareOp = m_PipelineSpecification.bDepthTest ? m_PipelineSpecification.DepthCompareOp : VK_COMPARE_OP_ALWAYS;
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
    VkPipelineDynamicStateCreateInfo DynamicState = {};
    DynamicState.sType                            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicState.dynamicStateCount                = static_cast<uint32_t>(DynamicStates.size());
    DynamicState.pDynamicStates                   = DynamicStates.data();

    VkGraphicsPipelineCreateInfo PipelineCreateInfo = {};
    PipelineCreateInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineCreateInfo.layout                       = m_PipelineLayout;
    PipelineCreateInfo.renderPass                   = m_PipelineSpecification.RenderPass;
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
        vkCreateGraphicsPipelines(Context.GetDevice()->GetLogicalDevice(), VK_NULL_HANDLE, 1, &PipelineCreateInfo, nullptr, &m_Pipeline),
        "Failed to create pipeline!");
}

void VulkanPipeline::Destroy()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    vkDestroyPipeline(Context.GetDevice()->GetLogicalDevice(), m_Pipeline, nullptr);
    vkDestroyPipelineLayout(Context.GetDevice()->GetLogicalDevice(), m_PipelineLayout, nullptr);
}

}  // namespace Eclipse
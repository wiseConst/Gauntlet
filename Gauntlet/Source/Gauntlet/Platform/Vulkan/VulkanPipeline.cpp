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

VulkanPipeline::VulkanPipeline(const PipelineSpecification& pipelineSpecification) : m_Specification(pipelineSpecification)
{
    Invalidate();
}

VulkanPipeline::~VulkanPipeline()
{
    // Destroy();
}

void VulkanPipeline::Invalidate()
{
    if (m_Handle) Destroy();

    CreateLayout();

    const float PipelineCreationBegin = Application::Get().GetTimeNow();
    Create();
    const float PipelineCreationEnd = Application::Get().GetTimeNow();
    LOG_INFO("Took %0.3f ms to create <%s> pipeline!", (PipelineCreationEnd - PipelineCreationBegin) * 1000.0f,
             m_Specification.Name.data());

    // Destroying linked shaders here.
    // TODO: Maybe I shouldn't? What if I need to recreate pipeline? Shaders are already deleted.
}

void VulkanPipeline::CreateLayout()
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    const auto vulkanShader = std::static_pointer_cast<VulkanShader>(m_Specification.Shader);
    GNT_ASSERT(vulkanShader, "Invalid shader!");

    VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    PipelineLayoutCreateInfo.setLayoutCount             = static_cast<uint32_t>(vulkanShader->GetDescriptorSetLayouts().size());
    PipelineLayoutCreateInfo.pSetLayouts                = vulkanShader->GetDescriptorSetLayouts().data();
    PipelineLayoutCreateInfo.pushConstantRangeCount     = static_cast<uint32_t>(vulkanShader->GetPushConstants().size());
    PipelineLayoutCreateInfo.pPushConstantRanges        = vulkanShader->GetPushConstants().data();

    VK_CHECK(vkCreatePipelineLayout(context.GetDevice()->GetLogicalDevice(), &PipelineLayoutCreateInfo, nullptr, &m_Layout),
             "Failed to create pipeline layout!");
}

void VulkanPipeline::Create()
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetDevice()->IsValid(), "Vulkan device is not valid!");
    GNT_ASSERT(context.GetSwapchain()->IsValid(), "Vulkan swapchain is not valid!");
    GNT_ASSERT(m_Specification.Shader, "Not valid shader passed!");
    auto& logicalDevice = context.GetDevice()->GetLogicalDevice();

    // Creating and validating pipeline cache
    {
        if (!std::filesystem::exists("Resources/Cached/Pipelines")) std::filesystem::create_directories("Resources/Cached/Pipelines");

        std::vector<uint8_t> CacheData = Utility::LoadDataFromDisk("Resources/Cached/Pipelines/" + m_Specification.Name + ".cache");
        VkPipelineCacheCreateInfo PipelineCacheCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
        if (!CacheData.empty())
        {
            bool bSamePipelineUUID = true;
            for (uint16_t i = 0; i < VK_UUID_SIZE; ++i)
                if (context.GetDevice()->GetGPUProperties().pipelineCacheUUID[i] != CacheData[16 + i]) bSamePipelineUUID = false;

            bool bSameVendorID = true;
            bool bSameDeviceID = true;
            for (uint16_t i = 0; i < 4; ++i)
            {
                if (CacheData[8 + i] != ((context.GetDevice()->GetGPUProperties().vendorID >> (8 * i)) & 0xff)) bSameVendorID = false;
                if (CacheData[12 + i] != ((context.GetDevice()->GetGPUProperties().deviceID >> (8 * i)) & 0xff)) bSameDeviceID = false;
            }

            if (bSamePipelineUUID && bSameVendorID && bSameDeviceID)
            {
                PipelineCacheCreateInfo.initialDataSize = CacheData.size();
                PipelineCacheCreateInfo.pInitialData    = CacheData.data();
            }
        }
        VK_CHECK(vkCreatePipelineCache(logicalDevice, &PipelineCacheCreateInfo, nullptr, &m_Cache), "Failed to create pipeline cache!");
    }

    switch (m_Specification.PipelineType)
    {
        case Gauntlet::EPipelineType::PIPELINE_TYPE_GRAPHICS:
        {
            // Contains the configuration for what kind of topology will be drawn.
            VkPipelineInputAssemblyStateCreateInfo InputAssemblyState = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
            InputAssemblyState.primitiveRestartEnable                 = m_Specification.PrimitiveRestartEnable;
            InputAssemblyState.topology = Utility::GauntletPrimitiveTopologyToVulkan(m_Specification.PrimitiveTopology);

            const auto vulkanShader = std::static_pointer_cast<VulkanShader>(m_Specification.Shader);
            GNT_ASSERT(vulkanShader, "Invalid shader!");

            std::vector<VkPipelineShaderStageCreateInfo> ShaderStages(vulkanShader->GetStages().size());
            for (size_t i = 0; i < ShaderStages.size(); ++i)
            {
                ShaderStages[i].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                ShaderStages[i].pName  = "main";
                ShaderStages[i].module = vulkanShader->GetStages()[i].Module;
                ShaderStages[i].stage  = Utility::GauntletShaderStageToVulkan(vulkanShader->GetStages()[i].Stage);
            }

            // VertexInputState
            VkPipelineVertexInputStateCreateInfo VertexInputState = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

            BufferLayout layout =
                m_Specification.Layout.GetElements().size() > 0 ? m_Specification.Layout : vulkanShader->GetVertexBufferLayout();

            std::vector<VkVertexInputAttributeDescription> shaderAttributeDescriptions;
            shaderAttributeDescriptions.reserve(layout.GetElements().size());

            for (uint32_t i = 0; i < shaderAttributeDescriptions.capacity(); ++i)
            {
                const auto& CurrentBufferElement = layout.GetElements()[i];
                shaderAttributeDescriptions.emplace_back(i, 0, Utility::GauntletShaderDataTypeToVulkan(CurrentBufferElement.Type),
                                                         CurrentBufferElement.Offset);
            }
            VertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(shaderAttributeDescriptions.size());
            VertexInputState.pVertexAttributeDescriptions    = shaderAttributeDescriptions.data();

            const auto vertexBindingDescription = Utility::GetShaderBindingDescription(0, layout.GetStride());
            if (shaderAttributeDescriptions.size() > 0)
            {
                VertexInputState.vertexBindingDescriptionCount = 1;
                VertexInputState.pVertexBindingDescriptions    = &vertexBindingDescription;
            }

            // TessellationState
            VkPipelineTessellationStateCreateInfo TessellationState = {VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO};

            // Configuration for the fixed-function rasterization. In here is where we enable or disable backface culling, and set line
            // width or wireframe drawing.
            VkPipelineRasterizationStateCreateInfo RasterizationState = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
            RasterizationState.cullMode                               = Utility::GauntletCullModeToVulkan(m_Specification.CullMode);
            RasterizationState.lineWidth                              = m_Specification.LineWidth;
            RasterizationState.polygonMode                            = Utility::GauntletPolygonModeToVulkan(m_Specification.PolygonMode);
            RasterizationState.frontFace                              = Utility::GauntletFrontFaceToVulkan(m_Specification.FrontFace);

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

            // TODO: Make it configurable
            VkPipelineMultisampleStateCreateInfo MultisampleState = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
            MultisampleState.sampleShadingEnable                  = VK_FALSE;
            MultisampleState.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;
            MultisampleState.minSampleShading                     = 1.0f;
            MultisampleState.alphaToCoverageEnable                = VK_FALSE;
            MultisampleState.alphaToOneEnable                     = VK_FALSE;

            // TODO: Make it configurable
            std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates;

            auto vulkanFramebuffer   = std::static_pointer_cast<VulkanFramebuffer>(m_Specification.TargetFramebuffer);
            uint32_t attachmentCount = (uint32_t)vulkanFramebuffer->GetAttachments().size();
            if (vulkanFramebuffer->GetDepthAttachment()) --attachmentCount;

            for (uint32_t i = 0; i < attachmentCount; ++i)
            {
                VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
                colorBlendAttachmentState.blendEnable                         = m_Specification.bBlendEnable;
                colorBlendAttachmentState.colorWriteMask                      = 0xf;
                // Color.rgb = (src.a * src.rgb) + ((1-src.a) * dst.rgb)
                colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                colorBlendAttachmentState.colorBlendOp        = VK_BLEND_OP_ADD;
                // Optional
                colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                colorBlendAttachmentState.alphaBlendOp        = VK_BLEND_OP_ADD;
                colorBlendAttachmentStates.push_back(colorBlendAttachmentState);
            }

            // Controls how this pipeline blends into a given attachment.
            /*
             * Setup dummy color blending. We aren't using transparent objects yet
             * the blending is just "no blend", but we do write to the color attachment
             */
            VkPipelineColorBlendStateCreateInfo ColorBlendState = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
            ColorBlendState.attachmentCount                     = static_cast<uint32_t>(colorBlendAttachmentStates.size());
            ColorBlendState.pAttachments                        = colorBlendAttachmentStates.data();
            ColorBlendState.logicOpEnable                       = VK_FALSE;
            ColorBlendState.logicOp                             = VK_LOGIC_OP_COPY;

            // Unlike OpenGL([-1, 1]), Vulkan has depth range [0, 1]
            // Also flip up the viewport and set frontface to ccw
            VkViewport Viewport = {};
            Viewport.height     = -static_cast<float>(context.GetSwapchain()->GetImageExtent().height);
            Viewport.width      = static_cast<float>(context.GetSwapchain()->GetImageExtent().width);
            Viewport.minDepth   = 0.0f;
            Viewport.maxDepth   = 1.0f;
            Viewport.x          = 0.0f;
            Viewport.y          = static_cast<float>(context.GetSwapchain()->GetImageExtent().height);

            VkRect2D Scissor = {};
            Scissor.offset   = {0, 0};
            Scissor.extent   = context.GetSwapchain()->GetImageExtent();

            // TODO: Make it configurable?
            VkPipelineViewportStateCreateInfo ViewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
            ViewportState.viewportCount                     = 1;
            ViewportState.pViewports                        = &Viewport;
            ViewportState.scissorCount                      = 1;
            ViewportState.pScissors                         = &Scissor;

            VkPipelineDepthStencilStateCreateInfo DepthStencilState = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
            DepthStencilState.depthTestEnable                       = m_Specification.bDepthTest ? VK_TRUE : VK_FALSE;
            DepthStencilState.depthWriteEnable                      = m_Specification.bDepthWrite ? VK_TRUE : VK_FALSE;
            DepthStencilState.depthCompareOp =
                m_Specification.bDepthTest ? Utility::GauntletCompareOpToVulkan(m_Specification.DepthCompareOp) : VK_COMPARE_OP_ALWAYS;
            DepthStencilState.depthBoundsTestEnable = VK_FALSE;
            DepthStencilState.stencilTestEnable     = VK_FALSE;

            /*
             * MIN && MAX depth bounds lets us cap the depth test.
             * If the depth is outside of bounds, the pixel will be skipped.
             */
            DepthStencilState.minDepthBounds = 0.0f;
            DepthStencilState.maxDepthBounds = 1.0f;

            std::vector<VkDynamicState> DynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

            if (m_Specification.bDynamicPolygonMode && !RENDERDOC_DEBUG) DynamicStates.push_back(VK_DYNAMIC_STATE_POLYGON_MODE_EXT);

            // TODO: Make it configurable?
            VkPipelineDynamicStateCreateInfo DynamicState = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
            DynamicState.dynamicStateCount                = static_cast<uint32_t>(DynamicStates.size());
            DynamicState.pDynamicStates                   = DynamicStates.data();

            VkGraphicsPipelineCreateInfo pipelineCreateInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
            pipelineCreateInfo.layout                       = m_Layout;
            pipelineCreateInfo.renderPass                   = vulkanFramebuffer->GetRenderPass();
            pipelineCreateInfo.pInputAssemblyState          = &InputAssemblyState;
            pipelineCreateInfo.stageCount                   = static_cast<uint32_t>(ShaderStages.size());
            pipelineCreateInfo.pStages                      = ShaderStages.data();
            pipelineCreateInfo.pVertexInputState            = &VertexInputState;
            pipelineCreateInfo.pRasterizationState          = &RasterizationState;
            pipelineCreateInfo.pTessellationState           = &TessellationState;
            pipelineCreateInfo.pViewportState               = &ViewportState;
            pipelineCreateInfo.pMultisampleState            = &MultisampleState;
            pipelineCreateInfo.pDepthStencilState           = &DepthStencilState;
            pipelineCreateInfo.pColorBlendState             = &ColorBlendState;
            pipelineCreateInfo.pDynamicState                = &DynamicState;

            VK_CHECK(vkCreateGraphicsPipelines(logicalDevice, m_Cache, 1, &pipelineCreateInfo, VK_NULL_HANDLE, &m_Handle),
                     "Failed to create GRAPHICS pipeline!");

            vulkanShader->DestroyModulesAndReflectionGarbage();
            break;
        }
        case Gauntlet::EPipelineType::PIPELINE_TYPE_COMPUTE:
        {
            VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
            shaderStageCreateInfo.stage                           = VK_SHADER_STAGE_COMPUTE_BIT;

            VkComputePipelineCreateInfo pipelineCreateInfo = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
            pipelineCreateInfo.layout                      = m_Layout;
            pipelineCreateInfo.stage                       = shaderStageCreateInfo;

            VK_CHECK(vkCreateComputePipelines(logicalDevice, m_Cache, 1, &pipelineCreateInfo, VK_NULL_HANDLE, &m_Handle),
                     "Failed to create COMPUTE pipeline!");
            break;
        }
        case Gauntlet::EPipelineType::PIPELINE_TYPE_RAY_TRACING:
        {
            VkRayTracingPipelineCreateInfoKHR pipelineCreateInfo = {VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR};
            pipelineCreateInfo.layout                            = m_Layout;

            VkDeferredOperationKHR op = {};  // ?

            VK_CHECK(vkCreateRayTracingPipelinesKHR(logicalDevice, op, m_Cache, 1, &pipelineCreateInfo, VK_NULL_HANDLE, &m_Handle),
                     "Failed to create RAY_TRACING pipeline!");
            break;
        }
        default: LOG_WARN("Pipeline type default?"); break;
    }
    GNT_ASSERT("Unknown pipeline type!");

    {
        GNT_ASSERT(Utility::DropPipelineCacheToDisk(logicalDevice, m_Cache,
                                                    "Resources/Cached/Pipelines/" + m_Specification.Name + ".cache") == VK_TRUE,
                   "Failed to save pipeline cache to disk!");

        vkDestroyPipelineCache(logicalDevice, m_Cache, nullptr);
    }
}

void VulkanPipeline::Destroy()
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetDevice()->IsValid(), "Vulkan device is not valid!");
    auto& logicalDevice = context.GetDevice()->GetLogicalDevice();

    vkDestroyPipelineLayout(logicalDevice, m_Layout, nullptr);
    vkDestroyPipeline(logicalDevice, m_Handle, nullptr);
}

const VkShaderStageFlags VulkanPipeline::GetPushConstantsShaderStageFlags(const uint32_t Index) const
{
    auto vulkanShader = std::static_pointer_cast<VulkanShader>(m_Specification.Shader);
    return vulkanShader->GetPushConstants()[Index].stageFlags;
}

const uint32_t VulkanPipeline::GetPushConstantsSize(const uint32_t Index) const
{
    auto vulkanShader = std::static_pointer_cast<VulkanShader>(m_Specification.Shader);
    GNT_ASSERT(!vulkanShader->GetPushConstants().empty(), "No push constants entry in the shader!");
    return vulkanShader->GetPushConstants()[Index].size;
}

}  // namespace Gauntlet
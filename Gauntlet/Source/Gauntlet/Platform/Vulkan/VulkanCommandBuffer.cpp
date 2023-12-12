#include "GauntletPCH.h"
#include "VulkanCommandBuffer.h"

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanPipeline.h"
#include "VulkanDevice.h"

namespace Gauntlet
{

static constexpr uint32_t s_MaxTimestampQueries = 16;

static VkCommandBufferLevel GauntletCommandBufferLevelToVulkan(ECommandBufferLevel level)
{
    switch (level)
    {
        case Gauntlet::ECommandBufferLevel::COMMAND_BUFFER_LEVEL_PRIMARY: return VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        case Gauntlet::ECommandBufferLevel::COMMAND_BUFFER_LEVEL_SECONDARY: return VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        default: break;
    }
    GNT_ASSERT(false, "Unknown command buffer level!");

    return VK_COMMAND_BUFFER_LEVEL_PRIMARY;
}

VulkanCommandBuffer::VulkanCommandBuffer(ECommandBufferType type, ECommandBufferLevel level) : m_Type(type), m_Level(level)
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    context.GetDevice()->AllocateCommandBuffer(m_CommandBuffer, type, GauntletCommandBufferLevelToVulkan(m_Level));

    CreateSyncResourcesAndQueries();
}

void VulkanCommandBuffer::CreateSyncResourcesAndQueries()
{
    auto& context                     = (VulkanContext&)VulkanContext::Get();
    VkFenceCreateInfo fenceCreateInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VK_CHECK(vkCreateFence(context.GetDevice()->GetLogicalDevice(), &fenceCreateInfo, VK_NULL_HANDLE, &m_SubmitFence),
             "Failed to create fence!");

    VkQueryPoolCreateInfo queryPoolCreateInfo = {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
    queryPoolCreateInfo.queryType             = VK_QUERY_TYPE_PIPELINE_STATISTICS;
    queryPoolCreateInfo.pipelineStatistics    = VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |   // Input assembly vertices
                                             VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |    // Input assembly primitives
                                             VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |    // Vertex shader invocations
                                             VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |  // Fragment shader invocations
                                             VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |         // Clipping invocations
                                             VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |          // Clipped primitives
                                             VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT |  //  Geometry Shader
                                             VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT |   //
                                             VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT |  // Tesselation
                                             VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT |
                                             VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT  // Compute shader invocations
#if VK_MESH_SHADING
                                             | VK_QUERY_PIPELINE_STATISTIC_TASK_SHADER_INVOCATIONS_BIT_EXT |  // MESH SHADING
                                             VK_QUERY_PIPELINE_STATISTIC_MESH_SHADER_INVOCATIONS_BIT_EXT
#endif
        ;

    // compute only case
    if (m_Type == ECommandBufferType::COMMAND_BUFFER_TYPE_COMPUTE)
        queryPoolCreateInfo.pipelineStatistics = VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;

    queryPoolCreateInfo.queryCount = 1;
    m_PipelineStatsResults.resize(VK_MESH_SHADING ? 13 : 11);
    VK_CHECK(vkCreateQueryPool(context.GetDevice()->GetLogicalDevice(), &queryPoolCreateInfo, nullptr, &m_PipelineStatisticsQueryPool),
             "Failed to create query pools!");

    queryPoolCreateInfo.queryCount         = s_MaxTimestampQueries;
    queryPoolCreateInfo.queryType          = VK_QUERY_TYPE_TIMESTAMP;
    queryPoolCreateInfo.pipelineStatistics = 0;

    m_TimestampResults.resize(s_MaxTimestampQueries);
    VK_CHECK(vkCreateQueryPool(context.GetDevice()->GetLogicalDevice(), &queryPoolCreateInfo, nullptr, &m_TimestampQueryPool),
             "Failed to create query pools!");
}

void VulkanCommandBuffer::BeginRecording(bool bOneTimeSubmit, const void* inheritanceInfo)
{
    VkCommandBufferBeginInfo commandBufferBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

    if (m_Level == ECommandBufferLevel::COMMAND_BUFFER_LEVEL_SECONDARY)
    {
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        GNT_ASSERT(inheritanceInfo, "Secondary command buffer requires inheritance info!");
    }

    if (bOneTimeSubmit) commandBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    commandBufferBeginInfo.pInheritanceInfo = (VkCommandBufferInheritanceInfo*)inheritanceInfo;
    VK_CHECK(vkBeginCommandBuffer(m_CommandBuffer, &commandBufferBeginInfo), "Failed to begin command buffer recording!");

    m_TimestampIndex = 0;
}

void VulkanCommandBuffer::Submit(bool bWaitAfterSubmit)
{
    VkSubmitInfo submitInfo       = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &m_CommandBuffer;

    auto& context = (VulkanContext&)VulkanContext::Get();
    VkQueue queue = VK_NULL_HANDLE;
    switch (m_Type)
    {
        case ECommandBufferType::COMMAND_BUFFER_TYPE_GRAPHICS:
        {
            queue = context.GetDevice()->GetGraphicsQueue();
            break;
        }
        case ECommandBufferType::COMMAND_BUFFER_TYPE_COMPUTE:
        {
            queue = context.GetDevice()->GetComputeQueue();
            break;
        }
        case ECommandBufferType::COMMAND_BUFFER_TYPE_TRANSFER:
        {
            queue = context.GetDevice()->GetTransferQueue();
            break;
        }
    }

    VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, m_SubmitFence), "Failed to submit command buffer!");

    if (bWaitAfterSubmit)
    {
        VK_CHECK(vkWaitForFences(context.GetDevice()->GetLogicalDevice(), 1, &m_SubmitFence, VK_TRUE, UINT64_MAX),
                 "Failed to wait for fence!");
        VK_CHECK(vkResetFences(context.GetDevice()->GetLogicalDevice(), 1, &m_SubmitFence), "Failed to reset fence!");
    }

    // A note on the flags used:
    //	VK_QUERY_RESULT_64_BIT: Results will have 64 bits. As time stamp values are on nano-seconds, this flag should always be used to
    // avoid 32 bit overflows
    //  VK_QUERY_RESULT_WAIT_BIT: Since we want to immediately display the results, we use this flag to have the CPU wait until the
    //  results are available (it stalls my cpu)
    {
        vkGetQueryPoolResults(context.GetDevice()->GetLogicalDevice(), m_PipelineStatisticsQueryPool, 0, 1,
                              m_PipelineStatsResults.size() * sizeof(m_PipelineStatsResults[0]), m_PipelineStatsResults.data(),
                              sizeof(m_PipelineStatsResults[0]), VK_QUERY_RESULT_64_BIT);

        vkGetQueryPoolResults(context.GetDevice()->GetLogicalDevice(), m_TimestampQueryPool, 0, s_MaxTimestampQueries,
                              m_TimestampResults.size() * sizeof(m_TimestampResults[0]), m_TimestampResults.data(),
                              sizeof(m_TimestampResults[0]), VK_QUERY_RESULT_64_BIT);
    }
}

const std::vector<std::string> VulkanCommandBuffer::GetPipelineStatisticsStrings() const
{
    const std::vector<std::string> pipelineStatisticsStrings = {
        "Input assembly vertex count         ",
        "Input assembly primitives count     ",
        "Vertex shader invocations           ",
        "Geometry Shader Invocations         ",
        "Geometry Shader Primitives          ",
        "Clipping stage primitives processed ",
        "Clipping stage primitives output    ",
        "Fragment shader invocations         ",
        "Tess. control shader patches        ",
        "Tess. eval. shader invocations      ",
        "Compute shader invocations          ",
#if VK_MESH_SHADING
        "Task shader invocations             ",
        "Mesh shader invocations             ",
#endif
    };

    return pipelineStatisticsStrings;
}

void VulkanCommandBuffer::Destroy()
{
    if (!m_CommandBuffer) return;

    auto& context = (VulkanContext&)VulkanContext::Get();

    context.GetDevice()->FreeCommandBuffer(m_CommandBuffer, m_Type);

    vkDestroyFence(context.GetDevice()->GetLogicalDevice(), m_SubmitFence, VK_NULL_HANDLE);
    vkDestroyQueryPool(context.GetDevice()->GetLogicalDevice(), m_TimestampQueryPool, VK_NULL_HANDLE);
    vkDestroyQueryPool(context.GetDevice()->GetLogicalDevice(), m_PipelineStatisticsQueryPool, VK_NULL_HANDLE);
}

void VulkanCommandBuffer::BeginTimestamp(bool bStatisticsQuery)
{
    if (bStatisticsQuery)
    {
        vkCmdResetQueryPool(m_CommandBuffer, m_PipelineStatisticsQueryPool, 0, 1);
        vkCmdBeginQuery(m_CommandBuffer, m_PipelineStatisticsQueryPool, 0, 0);
        return;
    }

    if (m_TimestampIndex == 0)
    {
        vkCmdResetQueryPool(m_CommandBuffer, m_TimestampQueryPool, 0, static_cast<uint32_t>(m_TimestampResults.size()));
    }

    vkCmdWriteTimestamp(m_CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_TimestampQueryPool, m_TimestampIndex);

    if (m_TimestampIndex + 1 > s_MaxTimestampQueries) GNT_ASSERT(false);
    ++m_TimestampIndex;
}

void VulkanCommandBuffer::EndTimestamp(bool bStatisticsQuery)
{
    if (bStatisticsQuery)
    {
        vkCmdEndQuery(m_CommandBuffer, m_PipelineStatisticsQueryPool, 0);
        return;
    }

    vkCmdWriteTimestamp(m_CommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_TimestampQueryPool, m_TimestampIndex);

    if (m_TimestampIndex + 1 > s_MaxTimestampQueries) GNT_ASSERT(false);
    ++m_TimestampIndex;
}

void VulkanCommandBuffer::BeginDebugLabel(const char* commandBufferLabelName, const glm::vec4& labelColor) const
{
    if (!s_bEnableValidationLayers && !VK_FORCE_VALIDATION) return;

    VkDebugUtilsLabelEXT commandBufferLabelEXT = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    commandBufferLabelEXT.pLabelName           = commandBufferLabelName;
    commandBufferLabelEXT.color[0]             = labelColor.r;
    commandBufferLabelEXT.color[1]             = labelColor.g;
    commandBufferLabelEXT.color[2]             = labelColor.b;
    commandBufferLabelEXT.color[3]             = labelColor.a;

    vkCmdBeginDebugUtilsLabelEXT(m_CommandBuffer, &commandBufferLabelEXT);
}

void VulkanCommandBuffer::BindDescriptorSets(Ref<VulkanPipeline>& pipeline, const uint32_t firstSet, const uint32_t descriptorSetCount,
                                             VkDescriptorSet* descriptorSets, const uint32_t dynamicOffsetCount,
                                             uint32_t* dynamicOffsets) const
{
    VkPipelineBindPoint pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    switch (pipeline->GetSpecification().PipelineType)
    {
        case EPipelineType::PIPELINE_TYPE_COMPUTE: pipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE; break;
        case EPipelineType::PIPELINE_TYPE_GRAPHICS: pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; break;
        case EPipelineType::PIPELINE_TYPE_RAY_TRACING: pipelineBindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR; break;
    }

    vkCmdBindDescriptorSets(m_CommandBuffer, pipelineBindPoint, pipeline->GetLayout(), firstSet, descriptorSetCount, descriptorSets,
                            dynamicOffsetCount, dynamicOffsets);
}

void VulkanCommandBuffer::BindPipeline(Ref<VulkanPipeline>& pipeline) const
{
    auto& context         = (VulkanContext&)VulkanContext::Get();
    const auto& swapchain = context.GetSwapchain();
    GNT_ASSERT(swapchain->IsValid(), "Vulkan swapchain is not valid!");

    VkViewport viewport = {};
    viewport.x          = 0.0f;
    viewport.minDepth   = 0.0f;
    viewport.maxDepth   = 1.0f;

    viewport.y      = static_cast<float>(swapchain->GetImageExtent().height);
    viewport.width  = static_cast<float>(swapchain->GetImageExtent().width);
    viewport.height = -static_cast<float>(swapchain->GetImageExtent().height);

    VkRect2D scissor = {{0, 0}, swapchain->GetImageExtent()};
    if (auto& targetFramebuffer = pipeline->GetSpecification().TargetFramebuffer[context.GetCurrentFrameIndex()])
    {
        scissor.extent = VkExtent2D(targetFramebuffer->GetWidth(), targetFramebuffer->GetHeight());

        viewport.y      = static_cast<float>(scissor.extent.height);
        viewport.width  = static_cast<float>(scissor.extent.width);
        viewport.height = -static_cast<float>(scissor.extent.height);
    }

    if (pipeline->GetSpecification().bDynamicPolygonMode && !RENDERDOC_DEBUG)
        vkCmdSetPolygonModeEXT(m_CommandBuffer, Utility::GauntletPolygonModeToVulkan(pipeline->GetSpecification().PolygonMode));

    VkPipelineBindPoint pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    switch (pipeline->GetSpecification().PipelineType)
    {
        case EPipelineType::PIPELINE_TYPE_COMPUTE: pipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE; break;
        case EPipelineType::PIPELINE_TYPE_GRAPHICS: pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; break;
        case EPipelineType::PIPELINE_TYPE_RAY_TRACING: pipelineBindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR; break;
    }

    vkCmdBindPipeline(m_CommandBuffer, pipelineBindPoint, pipeline->Get());

    if (pipelineBindPoint != VK_PIPELINE_BIND_POINT_COMPUTE)
    {
        vkCmdSetViewport(m_CommandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(m_CommandBuffer, 0, 1, &scissor);
    }
}

void VulkanCommandBuffer::SetPipelinePolygonMode(Ref<VulkanPipeline>& pipeline, const EPolygonMode polygonMode) const
{
    pipeline->GetSpecification().PolygonMode = polygonMode;
}

}  // namespace Gauntlet
#include "GauntletPCH.h"
#include "VulkanCommandBuffer.h"

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanPipeline.h"
#include "VulkanDevice.h"
#include "VulkanCommandPool.h"

namespace Gauntlet
{

static constexpr uint32_t s_MaxTimestampQueries = 16;

VulkanCommandBuffer::VulkanCommandBuffer(ECommandBufferType type, bool bIsSeparate) : m_Type(type), m_bAllocatedSeparate(bIsSeparate)
{
    if (bIsSeparate)  // created from renderer, not from command pool
    {
        auto& context = (VulkanContext&)VulkanContext::Get();
        switch (type)
        {
            case ECommandBufferType::COMMAND_BUFFER_TYPE_GRAPHICS:
            {
                context.GetGraphicsCommandPool()->AllocateSingleCommandBuffer(m_CommandBuffer);
                break;
            }
            case ECommandBufferType::COMMAND_BUFFER_TYPE_COMPUTE:
            {
                context.GetComputeCommandPool()->AllocateSingleCommandBuffer(m_CommandBuffer);
                break;
            }
            case ECommandBufferType::COMMAND_BUFFER_TYPE_TRANSFER:
            {
                context.GetTransferCommandPool()->AllocateSingleCommandBuffer(m_CommandBuffer);
                break;
            }
        }
    }

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
                                             VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT |   // Compute shader invocations
                                             VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT |  //  Geometry Shader
                                             VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT |   //
                                             VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT |  // Tesselation
                                             VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT;

    queryPoolCreateInfo.queryCount = 1;
    m_StatsResults.resize(11);
    VK_CHECK(vkCreateQueryPool(context.GetDevice()->GetLogicalDevice(), &queryPoolCreateInfo, nullptr, &m_StatisticsQueryPool),
             "Failed to create query pools!");

    queryPoolCreateInfo.queryCount         = s_MaxTimestampQueries;
    queryPoolCreateInfo.queryType          = VK_QUERY_TYPE_TIMESTAMP;
    queryPoolCreateInfo.pipelineStatistics = 0;

    m_TimestampResults.resize(s_MaxTimestampQueries);
    VK_CHECK(vkCreateQueryPool(context.GetDevice()->GetLogicalDevice(), &queryPoolCreateInfo, nullptr, &m_TimestampQueryPool),
             "Failed to create query pools!");
}

void VulkanCommandBuffer::BeginRecording(const void* inheritanceInfo)
{
    VkCommandBufferBeginInfo commandBufferBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

    if (m_Level == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    else
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    commandBufferBeginInfo.pInheritanceInfo = (VkCommandBufferInheritanceInfo*)inheritanceInfo;
    VK_CHECK(vkBeginCommandBuffer(m_CommandBuffer, &commandBufferBeginInfo), "Failed to begin command buffer recording!");
    m_TimestampIndex = 0;
}

void VulkanCommandBuffer::Submit()
{
    VkSubmitInfo submitInfo       = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &m_CommandBuffer;

    // what if m_Type  is compute?
    auto& context = (VulkanContext&)VulkanContext::Get();
    VK_CHECK(vkQueueSubmit(context.GetDevice()->GetGraphicsQueue(), 1, &submitInfo, m_SubmitFence), "Failed to submit command buffer!");
    VK_CHECK(vkWaitForFences(context.GetDevice()->GetLogicalDevice(), 1, &m_SubmitFence, VK_TRUE, UINT64_MAX), "Failed to wait for fence!");
    VK_CHECK(vkResetFences(context.GetDevice()->GetLogicalDevice(), 1, &m_SubmitFence), "Failed to reset fence!");

    // A note on the flags used:
    //	VK_QUERY_RESULT_64_BIT: Results will have 64 bits. As time stamp values are on nano-seconds, this flag should always be used to
    // avoid 32 bit overflows
    //  VK_QUERY_RESULT_WAIT_BIT: Since we want to immediately display the results, we use this flag to have the CPU wait until the
    //  results are available (it stalls my cpu)

    {
        vkGetQueryPoolResults(context.GetDevice()->GetLogicalDevice(), m_StatisticsQueryPool, 0, 1,
                              m_StatsResults.size() * sizeof(m_StatsResults[0]), m_StatsResults.data(), sizeof(m_StatsResults[0]),
                              VK_QUERY_RESULT_64_BIT);

        vkGetQueryPoolResults(context.GetDevice()->GetLogicalDevice(), m_TimestampQueryPool, 0, s_MaxTimestampQueries,
                              m_TimestampResults.size() * sizeof(m_TimestampResults[0]), m_TimestampResults.data(),
                              sizeof(m_TimestampResults[0]), VK_QUERY_RESULT_64_BIT);
    }
}

void VulkanCommandBuffer::Destroy()
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    context.WaitDeviceOnFinish();

    if (m_bAllocatedSeparate)
    {
        if (m_Type == ECommandBufferType::COMMAND_BUFFER_TYPE_GRAPHICS)
            context.GetGraphicsCommandPool()->FreeSingleCommandBuffer(m_CommandBuffer);
        else if (m_Type == ECommandBufferType::COMMAND_BUFFER_TYPE_COMPUTE)
            context.GetComputeCommandPool()->FreeSingleCommandBuffer(m_CommandBuffer);
        else if (m_Type == ECommandBufferType::COMMAND_BUFFER_TYPE_TRANSFER)
            context.GetTransferCommandPool()->FreeSingleCommandBuffer(m_CommandBuffer);
    }

    vkDestroyFence(context.GetDevice()->GetLogicalDevice(), m_SubmitFence, VK_NULL_HANDLE);
    vkDestroyQueryPool(context.GetDevice()->GetLogicalDevice(), m_TimestampQueryPool, VK_NULL_HANDLE);
    vkDestroyQueryPool(context.GetDevice()->GetLogicalDevice(), m_StatisticsQueryPool, VK_NULL_HANDLE);
}

void VulkanCommandBuffer::BeginTimestamp(bool bStatisticsQuery)
{
    if (bStatisticsQuery)
    {
        vkCmdResetQueryPool(m_CommandBuffer, m_StatisticsQueryPool, 0, 1);
        vkCmdBeginQuery(m_CommandBuffer, m_StatisticsQueryPool, 0, 0);
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
        vkCmdEndQuery(m_CommandBuffer, m_StatisticsQueryPool, 0);
        return;
    }

    vkCmdWriteTimestamp(m_CommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_TimestampQueryPool, m_TimestampIndex);

    if (m_TimestampIndex + 1 > s_MaxTimestampQueries) GNT_ASSERT(false);
    ++m_TimestampIndex;
}

void VulkanCommandBuffer::BeginDebugLabel(const char* commandBufferLabelName, const glm::vec4& labelColor) const
{
    if (!s_bEnableValidationLayers) return;

    VkDebugUtilsLabelEXT CommandBufferLabelEXT = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    CommandBufferLabelEXT.pLabelName           = commandBufferLabelName;
    CommandBufferLabelEXT.color[0]             = labelColor.r;
    CommandBufferLabelEXT.color[1]             = labelColor.g;
    CommandBufferLabelEXT.color[2]             = labelColor.b;
    CommandBufferLabelEXT.color[3]             = labelColor.a;

    vkCmdBeginDebugUtilsLabelEXT(m_CommandBuffer, &CommandBufferLabelEXT);
}

void VulkanCommandBuffer::BindPipeline(Ref<VulkanPipeline>& pipeline, VkPipelineBindPoint pipelineBindPoint) const
{
    auto& Context         = (VulkanContext&)VulkanContext::Get();
    const auto& Swapchain = Context.GetSwapchain();
    GNT_ASSERT(Swapchain->IsValid(), "Vulkan swapchain is not valid!");

    VkViewport Viewport = {};
    Viewport.x          = 0.0f;
    Viewport.minDepth   = 0.0f;
    Viewport.maxDepth   = 1.0f;

    Viewport.y      = static_cast<float>(Swapchain->GetImageExtent().height);
    Viewport.width  = static_cast<float>(Swapchain->GetImageExtent().width);
    Viewport.height = -static_cast<float>(Swapchain->GetImageExtent().height);

    VkRect2D Scissor = {{0, 0}, Swapchain->GetImageExtent()};
    if (auto& targetFramebuffer = pipeline->GetSpecification().TargetFramebuffer)
    {
        Scissor.extent = VkExtent2D(targetFramebuffer->GetWidth(), targetFramebuffer->GetHeight());

        Viewport.y      = static_cast<float>(Scissor.extent.height);
        Viewport.width  = static_cast<float>(Scissor.extent.width);
        Viewport.height = -static_cast<float>(Scissor.extent.height);
    }

    if (pipeline->GetSpecification().bDynamicPolygonMode && !RENDERDOC_DEBUG)
        vkCmdSetPolygonModeEXT(m_CommandBuffer, Utility::GauntletPolygonModeToVulkan(pipeline->GetSpecification().PolygonMode));

    vkCmdBindPipeline(m_CommandBuffer, pipelineBindPoint, pipeline->Get());
    vkCmdSetViewport(m_CommandBuffer, 0, 1, &Viewport);
    vkCmdSetScissor(m_CommandBuffer, 0, 1, &Scissor);
}

void VulkanCommandBuffer::SetPipelinePolygonMode(Ref<VulkanPipeline>& pipeline, const EPolygonMode polygonMode) const
{
    pipeline->GetSpecification().PolygonMode = polygonMode;
}

}  // namespace Gauntlet
#include "GauntletPCH.h"
#include "VulkanCommandBuffer.h"

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanPipeline.h"

namespace Gauntlet
{
void VulkanCommandBuffer::BeginRecording(const VkCommandBufferUsageFlags commandBufferUsageFlags,
                                         const VkCommandBufferInheritanceInfo* inheritanceInfo) const
{
    VkCommandBufferBeginInfo commandBufferBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

    if (m_Level == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    else
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | commandBufferUsageFlags;

    commandBufferBeginInfo.pInheritanceInfo = inheritanceInfo;
    VK_CHECK(vkBeginCommandBuffer(m_CommandBuffer, &commandBufferBeginInfo), "Failed to begin command buffer recording!");
}

void VulkanCommandBuffer::BeginDebugLabel(const char* commandBufferLabelName, const glm::vec4& labelColor) const
{
    //if (!s_bEnableValidationLayers) return;

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
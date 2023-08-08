#include "GauntletPCH.h"
#include "VulkanCommandBuffer.h"

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanPipeline.h"

namespace Gauntlet
{
void VulkanCommandBuffer::BeginRecording(const VkCommandBufferUsageFlags InCommandBufferUsageFlags) const
{
    VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
    CommandBufferBeginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CommandBufferBeginInfo.flags                    = InCommandBufferUsageFlags;

    VK_CHECK(vkBeginCommandBuffer(m_CommandBuffer, &CommandBufferBeginInfo), "Failed to begin command buffer recording!");
}

void VulkanCommandBuffer::BeginDebugLabel(const char* InCommandBufferLabelName, const glm::vec4& InLabelColor) const
{
    if (s_bEnableValidationLayers)
    {
        VkDebugUtilsLabelEXT CommandBufferLabelEXT = {};
        CommandBufferLabelEXT.sType                = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        CommandBufferLabelEXT.pLabelName           = InCommandBufferLabelName;
        CommandBufferLabelEXT.color[0]             = InLabelColor.r;
        CommandBufferLabelEXT.color[1]             = InLabelColor.g;
        CommandBufferLabelEXT.color[2]             = InLabelColor.b;
        CommandBufferLabelEXT.color[3]             = InLabelColor.a;

        vkCmdBeginDebugUtilsLabelEXT(m_CommandBuffer, &CommandBufferLabelEXT);
    }
}

void VulkanCommandBuffer::BindPipeline(const Ref<VulkanPipeline>& InPipeline, VkPipelineBindPoint InPipelineBindPoint) const
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
    if (InPipeline->GetSpecification().TargetFramebuffer)
    {
        Scissor.extent = VkExtent2D(InPipeline->GetSpecification().TargetFramebuffer->GetWidth(),
                                    InPipeline->GetSpecification().TargetFramebuffer->GetHeight());

        Viewport.y      = static_cast<float>(Scissor.extent.height);
        Viewport.width  = static_cast<float>(Scissor.extent.width);
        Viewport.height = -static_cast<float>(Scissor.extent.height);
    }

    vkCmdBindPipeline(m_CommandBuffer, InPipelineBindPoint, InPipeline->Get());
    vkCmdSetViewport(m_CommandBuffer, 0, 1, &Viewport);
    vkCmdSetScissor(m_CommandBuffer, 0, 1, &Scissor);
}

}  // namespace Gauntlet
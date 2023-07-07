#include "EclipsePCH.h"
#include "VulkanCommandBuffer.h"

#include "VulkanContext.h"
#include "VulkanSwapchain.h"

namespace Eclipse
{
void VulkanCommandBuffer::BeginRecording() const
{
    VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
    CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VK_CHECK(vkBeginCommandBuffer(m_CommandBuffer, &CommandBufferBeginInfo), "Failed to begin command buffer recording!");
}

void VulkanCommandBuffer::SetViewportAndScissors() const
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    const auto& Swapchain = Context.GetSwapchain();
    ELS_ASSERT(Swapchain->IsValid(), "Vulkan swapchain is not valid!");

    VkViewport Viewport = {};
    Viewport.x = 0.0f;
    Viewport.y = static_cast<float>(Swapchain->GetImageExtent().height);
    Viewport.width = static_cast<float>(Swapchain->GetImageExtent().width);
    Viewport.height = -static_cast<float>(Swapchain->GetImageExtent().height);
    Viewport.minDepth = 0.0f;
    Viewport.maxDepth = 1.0f;
    vkCmdSetViewport(m_CommandBuffer, 0, 1, &Viewport);

    VkRect2D Scissor = {{0.0f, 0.0f}, Swapchain->GetImageExtent()};
    vkCmdSetScissor(m_CommandBuffer, 0, 1, &Scissor);
}

}  // namespace Eclipse
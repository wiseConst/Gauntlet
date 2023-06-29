#include "EclipsePCH.h"
#include "VulkanCommandBuffer.h"

namespace Eclipse
{
void VulkanCommandBuffer::BeginRecording() const
{
    VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
    CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VK_CHECK(vkBeginCommandBuffer(m_CommandBuffer, &CommandBufferBeginInfo), "Failed to begin command buffer recording!");
}

}  // namespace Eclipse
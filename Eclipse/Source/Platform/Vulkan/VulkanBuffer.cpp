#include "EclipsePCH.h"
#include "VulkanBuffer.h"

namespace Eclipse
{

// VERTEX

VulkanVertexBuffer::VulkanVertexBuffer(const BufferInfo& InBufferInfo) : VertexBuffer(InBufferInfo) {}

VulkanVertexBuffer::~VulkanVertexBuffer() {}

void VulkanVertexBuffer::Destroy() {}

// INDEX

VulkanIndexBuffer::VulkanIndexBuffer(const BufferInfo& InBufferInfo) : IndexBuffer(InBufferInfo) {}

VulkanIndexBuffer::~VulkanIndexBuffer() {}

void VulkanIndexBuffer::Destroy() {}

namespace BufferUtils
{

}

}  // namespace Eclipse
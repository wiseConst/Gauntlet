#include "EclipsePCH.h"
#include "VulkanBuffer.h"

#include "VulkanContext.h"
#include "VulkanAllocator.h"

/*
 * In the very beginning all our data is stored in RAM,
 * to move it to CPU memory we have to:
 * 1) Map CPU memory(get pointer to the CPU memory - HOST_VISIBLE).
 * 2) Copy data to the mapped memory.
 *
 * Mapping and then unmapping the pointer lets the driver know that
 * the write is finished, and will be safer.
 */

namespace Eclipse
{

static VkBufferUsageFlags EclipseBufferUsageToVulkan(EBufferUsage InBufferUsage)
{
    switch (InBufferUsage)
    {
        case EBufferUsage::INDEX_BUFFER: return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        case EBufferUsage::VERTEX_BUFFER: return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        case EBufferUsage::UNIFORM_BUFFER: return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        case EBufferUsage::TRANSFER_DST: return VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        case EBufferUsage::STAGING_BUFFER: return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        case EBufferUsage::NONE:
        {
            ELS_ASSERT(false, "BufferUsage is NONE!");
            return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        }
    }

    ELS_ASSERT(false, "Unknown buffer usage flag!");
    return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
}

static void CreateBuffer(const BufferInfo& InBufferInfo, AllocatedBuffer& InAllocatedBuffer)
{
    VkBufferCreateInfo BufferCreateInfo = {};
    BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.usage = EclipseBufferUsageToVulkan(InBufferInfo.Usage);
    BufferCreateInfo.size = InBufferInfo.Size;
    BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    auto& Context = (VulkanContext&)VulkanContext::Get();
    InAllocatedBuffer.Allocation = Context.GetAllocator()->CreateBuffer(BufferCreateInfo, &InAllocatedBuffer.Buffer);
}

// VERTEX

VulkanVertexBuffer::VulkanVertexBuffer(const BufferInfo& InBufferInfo) : VertexBuffer(InBufferInfo), m_VertexCount(InBufferInfo.Count)
{
    CreateBuffer(InBufferInfo, m_AllocatedBuffer);
    auto& Context = (VulkanContext&)VulkanContext::Get();

    // Get pointer to the CPU memory(HOST_VISIBLE)
    void* Mapped = Context.GetAllocator()->Map(m_AllocatedBuffer.Allocation);
    memcpy(Mapped, InBufferInfo.Data, InBufferInfo.Size);
    Context.GetAllocator()->Unmap(m_AllocatedBuffer.Allocation);
}

void VulkanVertexBuffer::Destroy()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    Context.GetAllocator()->DestroyBuffer(m_AllocatedBuffer.Buffer, m_AllocatedBuffer.Allocation);
}

// INDEX

VulkanIndexBuffer::VulkanIndexBuffer(const BufferInfo& InBufferInfo) : IndexBuffer(InBufferInfo), m_IndicesCount(InBufferInfo.Count)
{
    CreateBuffer(InBufferInfo, m_AllocatedBuffer);
}

void VulkanIndexBuffer::Destroy()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    Context.GetAllocator()->DestroyBuffer(m_AllocatedBuffer.Buffer, m_AllocatedBuffer.Allocation);
}

}  // namespace Eclipse
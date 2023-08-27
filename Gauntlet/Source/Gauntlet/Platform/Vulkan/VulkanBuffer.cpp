#include "GauntletPCH.h"
#include "VulkanBuffer.h"

#include "VulkanContext.h"
#include "VulkanCommandPool.h"
#include "VulkanDevice.h"
#include "VulkanAllocator.h"
#include "VulkanUtility.h"

/*
 * Mapping and then unmapping the pointer lets the driver know that
 * the write is finished, and will be safer.
 *
 * The way I understand it:
 * Initially all data stored in system RAM, allocated staging buffer on CPU, copy data from RAM to cpu, next copy from CPU to GPU VRAM using
 * PCI-Express.
 */

namespace Gauntlet
{

namespace BufferUtils
{

VkBufferUsageFlags GauntletBufferUsageToVulkan(const EBufferUsage bufferUsage)
{
    VkBufferUsageFlags BufferUsageFlags = 0;

    if (bufferUsage & EBufferUsageFlags::INDEX_BUFFER) BufferUsageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (bufferUsage & EBufferUsageFlags::VERTEX_BUFFER) BufferUsageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (bufferUsage & EBufferUsageFlags::UNIFORM_BUFFER) BufferUsageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (bufferUsage & EBufferUsageFlags::TRANSFER_DST) BufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (bufferUsage & EBufferUsageFlags::STAGING_BUFFER) BufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    GNT_ASSERT(BufferUsageFlags != 0, "Unknown buffer usage flag!");
    return BufferUsageFlags;
}

void CreateBuffer(const EBufferUsage bufferUsage, const VkDeviceSize size, AllocatedBuffer& outAllocatedBuffer, VmaMemoryUsage memoryUsage)
{
    VkBufferCreateInfo BufferCreateInfo = {};
    BufferCreateInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.usage              = GauntletBufferUsageToVulkan(bufferUsage);
    BufferCreateInfo.size               = size;
    BufferCreateInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

    auto& Context                 = (VulkanContext&)VulkanContext::Get();
    outAllocatedBuffer.Allocation = Context.GetAllocator()->CreateBuffer(BufferCreateInfo, &outAllocatedBuffer.Buffer, memoryUsage);
}

void CopyBuffer(const VkBuffer& sourceBuffer, VkBuffer& destBuffer, const VkDeviceSize size)
{
    GRAPHICS_GUARD_LOCK;

    auto& Context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    auto CommandBuffer = Utility::BeginSingleTimeCommands(Context.GetTransferCommandPool()->Get(), Context.GetDevice()->GetLogicalDevice());

    VkBufferCopy CopyRegion = {};
    CopyRegion.size         = size;
    CopyRegion.srcOffset    = 0;  // Optional
    CopyRegion.dstOffset    = 0;  // Optional

    vkCmdCopyBuffer(CommandBuffer, sourceBuffer, destBuffer, 1, &CopyRegion);
    Utility::EndSingleTimeCommands(CommandBuffer, Context.GetTransferCommandPool()->Get(), Context.GetDevice()->GetTransferQueue(),
                                   Context.GetDevice()->GetLogicalDevice());
}

void DestroyBuffer(AllocatedBuffer& InBuffer)
{
    GRAPHICS_GUARD_LOCK;
    auto& Context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    Context.GetDevice()->WaitDeviceOnFinish();
    Context.GetAllocator()->DestroyBuffer(InBuffer.Buffer, InBuffer.Allocation);
    InBuffer.Buffer     = VK_NULL_HANDLE;
    InBuffer.Allocation = VK_NULL_HANDLE;
}

// Usually used for copying data to staging buffer
void CopyDataToBuffer(AllocatedBuffer& buffer, const VkDeviceSize dataSize, const void* data)
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(data, "Data you want to copy is not valid!");

    void* Mapped = Context.GetAllocator()->Map(buffer.Allocation);
    memcpy(Mapped, data, dataSize);
    Context.GetAllocator()->Unmap(buffer.Allocation);
}

}  // namespace BufferUtils

// VERTEX

VulkanVertexBuffer::VulkanVertexBuffer(BufferInfo& bufferInfo) : VertexBuffer(bufferInfo), m_VertexCount(bufferInfo.Count)
{
    // TODO: SetData() ?
}

void VulkanVertexBuffer::SetData(const void* data, const size_t size)
{
    if (m_AllocatedBuffer.Buffer != VK_NULL_HANDLE)
    {
        BufferUtils::DestroyBuffer(m_AllocatedBuffer);
    }

    AllocatedBuffer StagingBuffer = {};
    BufferUtils::CreateBuffer(EBufferUsageFlags::STAGING_BUFFER, size, StagingBuffer, VMA_MEMORY_USAGE_CPU_ONLY);
    BufferUtils::CopyDataToBuffer(StagingBuffer, size, data);

    BufferUtils::CreateBuffer(EBufferUsageFlags::VERTEX_BUFFER | EBufferUsageFlags::TRANSFER_DST, size, m_AllocatedBuffer,
                              VMA_MEMORY_USAGE_GPU_ONLY);

    BufferUtils::CopyBuffer(StagingBuffer.Buffer, m_AllocatedBuffer.Buffer, size);
    BufferUtils::DestroyBuffer(StagingBuffer);
}

void VulkanVertexBuffer::Destroy()
{
    BufferUtils::DestroyBuffer(m_AllocatedBuffer);
}

void VulkanVertexBuffer::SetStagedData(const AllocatedBuffer& stagingBuffer, const VkDeviceSize stagingBufferDataSize)
{
    // First call on this vertex buffer
    if (!m_AllocatedBuffer.Buffer)
    {
        BufferUtils::CreateBuffer(EBufferUsageFlags::VERTEX_BUFFER | EBufferUsageFlags::TRANSFER_DST, stagingBufferDataSize,
                                  m_AllocatedBuffer, VMA_MEMORY_USAGE_GPU_ONLY);
    }

    // Special case if created buffer above has less size than we need to put into it.
    auto& Context                    = (VulkanContext&)VulkanContext::Get();
    VmaAllocationInfo AllocationInfo = {};
    Context.GetAllocator()->QueryAllocationInfo(AllocationInfo, m_AllocatedBuffer.Allocation);

    // If new buffer data size greater than our current buffer size, then recreate it
    if (stagingBufferDataSize > AllocationInfo.size)
    {
        BufferUtils::DestroyBuffer(m_AllocatedBuffer);
        BufferUtils::CreateBuffer(EBufferUsageFlags::VERTEX_BUFFER | EBufferUsageFlags::TRANSFER_DST, stagingBufferDataSize,
                                  m_AllocatedBuffer, VMA_MEMORY_USAGE_GPU_ONLY);
    }

    BufferUtils::CopyBuffer(stagingBuffer.Buffer, m_AllocatedBuffer.Buffer, stagingBufferDataSize);
}

// INDEX

VulkanIndexBuffer::VulkanIndexBuffer(BufferInfo& bufferInfo) : IndexBuffer(bufferInfo), m_IndicesCount(bufferInfo.Count)
{
    AllocatedBuffer StagingBuffer = {};
    BufferUtils::CreateBuffer(EBufferUsageFlags::STAGING_BUFFER, bufferInfo.Size, StagingBuffer, VMA_MEMORY_USAGE_CPU_ONLY);
    BufferUtils::CopyDataToBuffer(StagingBuffer, bufferInfo.Size, bufferInfo.Data);

    BufferUtils::CreateBuffer(bufferInfo.Usage | EBufferUsageFlags::TRANSFER_DST, bufferInfo.Size, m_AllocatedBuffer,
                              VMA_MEMORY_USAGE_GPU_ONLY);
    BufferUtils::CopyBuffer(StagingBuffer.Buffer, m_AllocatedBuffer.Buffer, bufferInfo.Size);
    BufferUtils::DestroyBuffer(StagingBuffer);
}

void VulkanIndexBuffer::Destroy()
{
    BufferUtils::DestroyBuffer(m_AllocatedBuffer);
}

}  // namespace Gauntlet
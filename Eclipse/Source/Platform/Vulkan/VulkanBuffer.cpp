#include "EclipsePCH.h"
#include "VulkanBuffer.h"

#include "VulkanContext.h"
#include "VulkanCommandPool.h"
#include "VulkanDevice.h"
#include "VulkanAllocator.h"

/*
 * In the very beginning all our data is stored in RAM,
 * to move it to CPU memory we have to:
 * 1) Map CPU memory(get pointer to the CPU memory - HOST_VISIBLE).
 * 2) Copy data to the mapped memory.
 *
 * Mapping and then unmapping the pointer lets the driver know that
 * the write is finished, and will be safer.
 *
 * The way I understand it:
 * Initially all data stored in system RAM, allocated staging buffer on CPU, copy data from RAM to cpu, next copy from CPU to GPU VRAM using
 * PCI-Express.
 */

namespace Eclipse
{

namespace BufferUtils
{

VkBufferUsageFlags EclipseBufferUsageToVulkan(const EBufferUsage InBufferUsage)
{
    VkBufferUsageFlags BufferUsageFlags = 0;

    if (InBufferUsage & EBufferUsageFlags::INDEX_BUFFER) BufferUsageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (InBufferUsage & EBufferUsageFlags::VERTEX_BUFFER) BufferUsageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (InBufferUsage & EBufferUsageFlags::UNIFORM_BUFFER) BufferUsageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (InBufferUsage & EBufferUsageFlags::TRANSFER_DST) BufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (InBufferUsage & EBufferUsageFlags::STAGING_BUFFER) BufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    ELS_ASSERT(BufferUsageFlags != 0, "Unknown buffer usage flag!");
    return BufferUsageFlags;
}

void CreateBuffer(const EBufferUsage InBufferUsage, const VkDeviceSize InSize, AllocatedBuffer& InOutAllocatedBuffer,
                  VmaMemoryUsage InMemoryUsage)
{
    VkBufferCreateInfo BufferCreateInfo = {};
    BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.usage = EclipseBufferUsageToVulkan(InBufferUsage);
    BufferCreateInfo.size = InSize;
    BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    auto& Context = (VulkanContext&)VulkanContext::Get();
    InOutAllocatedBuffer.Allocation = Context.GetAllocator()->CreateBuffer(BufferCreateInfo, &InOutAllocatedBuffer.Buffer, InMemoryUsage);
}

void CopyBuffer(VkBuffer& InSourceBuffer, VkBuffer& InDestBuffer, const VkDeviceSize InSize)
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    auto CommandBuffer = Utility::BeginSingleTimeCommands(Context.GetTransferCommandPool()->Get(), Context.GetDevice()->GetLogicalDevice());

    VkBufferCopy CopyRegion = {};
    CopyRegion.size = InSize;
    CopyRegion.srcOffset = 0;  // Optional
    CopyRegion.dstOffset = 0;  // Optional

    vkCmdCopyBuffer(CommandBuffer, InSourceBuffer, InDestBuffer, 1, &CopyRegion);
    Utility::EndSingleTimeCommands(CommandBuffer, Context.GetTransferCommandPool()->Get(), Context.GetDevice()->GetTransferQueue(),
                                   Context.GetDevice()->GetLogicalDevice());
}

void DestroyBuffer(AllocatedBuffer& InBuffer)
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    Context.GetDevice()->WaitDeviceOnFinish();
    Context.GetAllocator()->DestroyBuffer(InBuffer.Buffer, InBuffer.Allocation);
    InBuffer.Buffer = VK_NULL_HANDLE;
    InBuffer.Allocation = VK_NULL_HANDLE;
}

}  // namespace BufferUtils

// VERTEX

VulkanVertexBuffer::VulkanVertexBuffer(BufferInfo& InBufferInfo) : VertexBuffer(InBufferInfo), m_VertexCount(InBufferInfo.Count) {}

void VulkanVertexBuffer::SetData(const void* InData, const size_t InDataSize)
{
    if (m_AllocatedBuffer.Buffer != VK_NULL_HANDLE)
    {
        BufferUtils::DestroyBuffer(m_AllocatedBuffer);
    }

    auto& Context = (VulkanContext&)VulkanContext::Get();

    AllocatedBuffer StagingBuffer = {};
    BufferUtils::CreateBuffer(EBufferUsageFlags::STAGING_BUFFER, InDataSize, StagingBuffer, VMA_MEMORY_USAGE_CPU_ONLY);

    ELS_ASSERT(InData, "Vertex buffer data is null!");
    void* Mapped = Context.GetAllocator()->Map(StagingBuffer.Allocation);
    memcpy(Mapped, InData, InDataSize);
    Context.GetAllocator()->Unmap(StagingBuffer.Allocation);

    BufferUtils::CreateBuffer(EBufferUsageFlags::VERTEX_BUFFER | EBufferUsageFlags::TRANSFER_DST, InDataSize, m_AllocatedBuffer,
                              VMA_MEMORY_USAGE_GPU_ONLY);

    BufferUtils::CopyBuffer(StagingBuffer.Buffer, m_AllocatedBuffer.Buffer, InDataSize);
    BufferUtils::DestroyBuffer(StagingBuffer);
}

void VulkanVertexBuffer::Destroy()
{
    BufferUtils::DestroyBuffer(m_AllocatedBuffer);
}

// INDEX

VulkanIndexBuffer::VulkanIndexBuffer(BufferInfo& InBufferInfo) : IndexBuffer(InBufferInfo), m_IndicesCount(InBufferInfo.Count)
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    AllocatedBuffer StagingBuffer = {};
    BufferUtils::CreateBuffer(EBufferUsageFlags::STAGING_BUFFER, InBufferInfo.Size, StagingBuffer, VMA_MEMORY_USAGE_CPU_ONLY);

    ELS_ASSERT(InBufferInfo.Data, "Index buffer data is null!");
    void* Mapped = Context.GetAllocator()->Map(StagingBuffer.Allocation);
    memcpy(Mapped, InBufferInfo.Data, InBufferInfo.Size);
    Context.GetAllocator()->Unmap(StagingBuffer.Allocation);

    BufferUtils::CreateBuffer(InBufferInfo.Usage | EBufferUsageFlags::TRANSFER_DST, InBufferInfo.Size, m_AllocatedBuffer,
                              VMA_MEMORY_USAGE_GPU_ONLY);
    BufferUtils::CopyBuffer(StagingBuffer.Buffer, m_AllocatedBuffer.Buffer, InBufferInfo.Size);
    BufferUtils::DestroyBuffer(StagingBuffer);
}

void VulkanIndexBuffer::Destroy()
{
    BufferUtils::DestroyBuffer(m_AllocatedBuffer);
}

}  // namespace Eclipse
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

VkBufferUsageFlags GauntletBufferUsageToVulkan(const EBufferUsage InBufferUsage)
{
    VkBufferUsageFlags BufferUsageFlags = 0;

    if (InBufferUsage & EBufferUsageFlags::INDEX_BUFFER) BufferUsageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (InBufferUsage & EBufferUsageFlags::VERTEX_BUFFER) BufferUsageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (InBufferUsage & EBufferUsageFlags::UNIFORM_BUFFER) BufferUsageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (InBufferUsage & EBufferUsageFlags::TRANSFER_DST) BufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (InBufferUsage & EBufferUsageFlags::STAGING_BUFFER) BufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    GNT_ASSERT(BufferUsageFlags != 0, "Unknown buffer usage flag!");
    return BufferUsageFlags;
}

void CreateBuffer(const EBufferUsage InBufferUsage, const VkDeviceSize InSize, AllocatedBuffer& InOutAllocatedBuffer,
                  VmaMemoryUsage InMemoryUsage)
{
    VkBufferCreateInfo BufferCreateInfo = {};
    BufferCreateInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.usage              = GauntletBufferUsageToVulkan(InBufferUsage);
    BufferCreateInfo.size               = InSize;
    BufferCreateInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

    auto& Context                   = (VulkanContext&)VulkanContext::Get();
    InOutAllocatedBuffer.Allocation = Context.GetAllocator()->CreateBuffer(BufferCreateInfo, &InOutAllocatedBuffer.Buffer, InMemoryUsage);
}

void CopyBuffer(const VkBuffer& InSourceBuffer, VkBuffer& InDestBuffer, const VkDeviceSize InSize)
{
    GRAPHICS_GUARD_LOCK;

    auto& Context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    auto CommandBuffer = Utility::BeginSingleTimeCommands(Context.GetTransferCommandPool()->Get(), Context.GetDevice()->GetLogicalDevice());

    VkBufferCopy CopyRegion = {};
    CopyRegion.size         = InSize;
    CopyRegion.srcOffset    = 0;  // Optional
    CopyRegion.dstOffset    = 0;  // Optional

    vkCmdCopyBuffer(CommandBuffer, InSourceBuffer, InDestBuffer, 1, &CopyRegion);
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
void CopyDataToBuffer(AllocatedBuffer& InBuffer, const VkDeviceSize InDataSize, const void* InData)
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(InData, "Data you want to copy is not valid!");

    void* Mapped = Context.GetAllocator()->Map(InBuffer.Allocation);
    memcpy(Mapped, InData, InDataSize);
    Context.GetAllocator()->Unmap(InBuffer.Allocation);
}

}  // namespace BufferUtils

// VERTEX

VulkanVertexBuffer::VulkanVertexBuffer(BufferInfo& InBufferInfo) : VertexBuffer(InBufferInfo), m_VertexCount(InBufferInfo.Count)
{
    // TODO: SetData() ?
}

void VulkanVertexBuffer::SetData(const void* InData, const size_t InDataSize)
{
    if (m_AllocatedBuffer.Buffer != VK_NULL_HANDLE)
    {
        BufferUtils::DestroyBuffer(m_AllocatedBuffer);
    }

    AllocatedBuffer StagingBuffer = {};
    BufferUtils::CreateBuffer(EBufferUsageFlags::STAGING_BUFFER, InDataSize, StagingBuffer, VMA_MEMORY_USAGE_CPU_ONLY);
    BufferUtils::CopyDataToBuffer(StagingBuffer, InDataSize, InData);

    BufferUtils::CreateBuffer(EBufferUsageFlags::VERTEX_BUFFER | EBufferUsageFlags::TRANSFER_DST, InDataSize, m_AllocatedBuffer,
                              VMA_MEMORY_USAGE_GPU_ONLY);

    BufferUtils::CopyBuffer(StagingBuffer.Buffer, m_AllocatedBuffer.Buffer, InDataSize);
    BufferUtils::DestroyBuffer(StagingBuffer);
}

void VulkanVertexBuffer::Destroy()
{
    BufferUtils::DestroyBuffer(m_AllocatedBuffer);
}

void VulkanVertexBuffer::SetStagedData(const AllocatedBuffer& InStagingBuffer, const VkDeviceSize InBufferDataSize)
{
    // First call on this vertex buffer
    if (!m_AllocatedBuffer.Buffer)
    {
        BufferUtils::CreateBuffer(EBufferUsageFlags::VERTEX_BUFFER | EBufferUsageFlags::TRANSFER_DST, InBufferDataSize, m_AllocatedBuffer,
                                  VMA_MEMORY_USAGE_GPU_ONLY);
    }

    // Special case if created buffer above has less size than we need to put into it.
    auto& Context                    = (VulkanContext&)VulkanContext::Get();
    VmaAllocationInfo AllocationInfo = {};
    Context.GetAllocator()->QueryAllocationInfo(AllocationInfo, m_AllocatedBuffer.Allocation);

    // If new buffer data size greater than our current buffer size, then recreate it
    if (InBufferDataSize > AllocationInfo.size)
    {
        BufferUtils::DestroyBuffer(m_AllocatedBuffer);
        BufferUtils::CreateBuffer(EBufferUsageFlags::VERTEX_BUFFER | EBufferUsageFlags::TRANSFER_DST, InBufferDataSize, m_AllocatedBuffer,
                                  VMA_MEMORY_USAGE_GPU_ONLY);
    }

    BufferUtils::CopyBuffer(InStagingBuffer.Buffer, m_AllocatedBuffer.Buffer, InBufferDataSize);
}

// INDEX

VulkanIndexBuffer::VulkanIndexBuffer(BufferInfo& InBufferInfo) : IndexBuffer(InBufferInfo), m_IndicesCount(InBufferInfo.Count)
{
    AllocatedBuffer StagingBuffer = {};
    BufferUtils::CreateBuffer(EBufferUsageFlags::STAGING_BUFFER, InBufferInfo.Size, StagingBuffer, VMA_MEMORY_USAGE_CPU_ONLY);
    BufferUtils::CopyDataToBuffer(StagingBuffer, InBufferInfo.Size, InBufferInfo.Data);

    BufferUtils::CreateBuffer(InBufferInfo.Usage | EBufferUsageFlags::TRANSFER_DST, InBufferInfo.Size, m_AllocatedBuffer,
                              VMA_MEMORY_USAGE_GPU_ONLY);
    BufferUtils::CopyBuffer(StagingBuffer.Buffer, m_AllocatedBuffer.Buffer, InBufferInfo.Size);
    BufferUtils::DestroyBuffer(StagingBuffer);
}

void VulkanIndexBuffer::Destroy()
{
    BufferUtils::DestroyBuffer(m_AllocatedBuffer);
}

}  // namespace Gauntlet
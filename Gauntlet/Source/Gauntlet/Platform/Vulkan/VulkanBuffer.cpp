#include "GauntletPCH.h"
#include "VulkanBuffer.h"

#include "VulkanContext.h"
#include "VulkanCommandPool.h"
#include "VulkanDevice.h"
#include "VulkanAllocator.h"
#include "VulkanUtility.h"

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

void CreateBuffer(const EBufferUsage bufferUsage, const VkDeviceSize size, VulkanAllocatedBuffer& outAllocatedBuffer,
                  VmaMemoryUsage memoryUsage)
{
    VkBufferCreateInfo BufferCreateInfo = {};
    BufferCreateInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.usage              = GauntletBufferUsageToVulkan(bufferUsage);
    BufferCreateInfo.size               = size;
    BufferCreateInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

    auto& context                 = (VulkanContext&)VulkanContext::Get();
    outAllocatedBuffer.Allocation = context.GetAllocator()->CreateBuffer(BufferCreateInfo, &outAllocatedBuffer.Buffer, memoryUsage);
}

void CopyBuffer(const VkBuffer& sourceBuffer, VkBuffer& destBuffer, const VkDeviceSize size)
{
    GRAPHICS_GUARD_LOCK;

    auto& context      = (VulkanContext&)VulkanContext::Get();
    auto CommandBuffer = Utility::BeginSingleTimeCommands(context.GetTransferCommandPool()->Get(), context.GetDevice()->GetLogicalDevice());

    VkBufferCopy CopyRegion = {};
    CopyRegion.size         = size;
    CopyRegion.srcOffset    = 0;  // Optional
    CopyRegion.dstOffset    = 0;  // Optional

    vkCmdCopyBuffer(CommandBuffer, sourceBuffer, destBuffer, 1, &CopyRegion);
    Utility::EndSingleTimeCommands(CommandBuffer, context.GetTransferCommandPool()->Get(), context.GetDevice()->GetTransferQueue(),
                                   context.GetDevice()->GetLogicalDevice(), context.GetUploadFence());
}

void DestroyBuffer(VulkanAllocatedBuffer& InBuffer)
{
    GRAPHICS_GUARD_LOCK;
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    context.WaitDeviceOnFinish();
    context.GetAllocator()->DestroyBuffer(InBuffer.Buffer, InBuffer.Allocation);
    InBuffer.Buffer     = VK_NULL_HANDLE;
    InBuffer.Allocation = VK_NULL_HANDLE;
}

// Usually used for copying data to staging buffer
void CopyDataToBuffer(VulkanAllocatedBuffer& buffer, const VkDeviceSize dataSize, const void* data)
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(data, "Data you want to copy is not valid!");

    void* Mapped = context.GetAllocator()->Map(buffer.Allocation);
    memcpy(Mapped, data, dataSize);
    context.GetAllocator()->Unmap(buffer.Allocation);
}

}  // namespace BufferUtils

// VERTEX

VulkanVertexBuffer::VulkanVertexBuffer(BufferInfo& bufferInfo)
    : VertexBuffer(bufferInfo), m_VertexCount(bufferInfo.Count), m_Layout(bufferInfo.Layout)
{
}

void VulkanVertexBuffer::SetData(const void* data, const size_t size)
{
    if (m_AllocatedBuffer.Buffer != VK_NULL_HANDLE)
    {
        BufferUtils::DestroyBuffer(m_AllocatedBuffer);
    }

    auto& stagingBuffer = Renderer::GetStorageData().UploadHeap;
    stagingBuffer->SetData(data, size);

    BufferUtils::CreateBuffer(EBufferUsageFlags::VERTEX_BUFFER | EBufferUsageFlags::TRANSFER_DST, size, m_AllocatedBuffer,
                              VMA_MEMORY_USAGE_GPU_ONLY);

    VkBuffer stagingBufferRaw = (VkBuffer)stagingBuffer->Get();
    GNT_ASSERT(stagingBufferRaw);
    BufferUtils::CopyBuffer(stagingBufferRaw, m_AllocatedBuffer.Buffer, size);
}

void VulkanVertexBuffer::Destroy()
{
    if (!m_AllocatedBuffer.Allocation)
    {
        LOG_TRACE("Unused vulkan buffer! No need to destroy, returning.");
        return;
    }

    BufferUtils::DestroyBuffer(m_AllocatedBuffer);
}

void VulkanVertexBuffer::SetStagedData(const Ref<StagingBuffer>& stagingBuffer, const uint64_t stagingBufferDataSize)
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

    VkBuffer stagingBufferRaw = (VkBuffer)stagingBuffer->Get();
    GNT_ASSERT(stagingBufferRaw);
    BufferUtils::CopyBuffer(stagingBufferRaw, m_AllocatedBuffer.Buffer, stagingBufferDataSize);
}

// INDEX

VulkanIndexBuffer::VulkanIndexBuffer(BufferInfo& bufferInfo) : IndexBuffer(bufferInfo), m_IndicesCount(bufferInfo.Count)
{
    GNT_ASSERT(bufferInfo.Data && bufferInfo.Size > 0);

    auto& stagingBuffer = Renderer::GetStorageData().UploadHeap;
    stagingBuffer->SetData(bufferInfo.Data, bufferInfo.Size);

    BufferUtils::CreateBuffer(bufferInfo.Usage | EBufferUsageFlags::TRANSFER_DST, bufferInfo.Size, m_AllocatedBuffer,
                              VMA_MEMORY_USAGE_GPU_ONLY);

    VkBuffer stagingBufferRaw = (VkBuffer)stagingBuffer->Get();
    GNT_ASSERT(stagingBufferRaw);
    BufferUtils::CopyBuffer(stagingBufferRaw, m_AllocatedBuffer.Buffer, bufferInfo.Size);
}

void VulkanIndexBuffer::Destroy()
{
    BufferUtils::DestroyBuffer(m_AllocatedBuffer);
}

// UNIFORM

VulkanUniformBuffer::VulkanUniformBuffer(const uint64_t bufferSize) : m_Size(bufferSize)
{
    m_bAreMapped.resize(FRAMES_IN_FLIGHT);
    for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        m_bAreMapped[frame] = false;

    m_AllocatedBuffers.resize(FRAMES_IN_FLIGHT);
    for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        BufferUtils::CreateBuffer(EBufferUsageFlags::UNIFORM_BUFFER, bufferSize, m_AllocatedBuffers[frame], VMA_MEMORY_USAGE_CPU_ONLY);
}

void VulkanUniformBuffer::Destroy()
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    for (uint32_t i = 0; i < m_AllocatedBuffers.size(); ++i)
    {
        if (m_bAreMapped[i])
        {
            context.GetAllocator()->Unmap(m_AllocatedBuffers[i].Allocation);
            m_bAreMapped[i] = false;
        }

        BufferUtils::DestroyBuffer(m_AllocatedBuffers[i]);
    }
    m_Size = 0;
}

void VulkanUniformBuffer::MapPersistent()
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
    {
        if (m_bAreMapped[frame]) continue;

        context.GetAllocator()->Map(m_AllocatedBuffers[frame].Allocation);
        m_bAreMapped[frame] = true;
    }
}

void* VulkanUniformBuffer::RetrieveMapped()
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    if (m_bAreMapped[GraphicsContext::Get().GetCurrentFrameIndex()])
    {
        VmaAllocationInfo allocationInfo = {};
        context.GetAllocator()->QueryAllocationInfo(allocationInfo, m_AllocatedBuffers[context.GetCurrentFrameIndex()].Allocation);
        return allocationInfo.pMappedData;
    }

    m_bAreMapped[GraphicsContext::Get().GetCurrentFrameIndex()] = true;
    return context.GetAllocator()->Map(m_AllocatedBuffers[GraphicsContext::Get().GetCurrentFrameIndex()].Allocation);
}

void VulkanUniformBuffer::Unmap()
{
    if (!m_bAreMapped[GraphicsContext::Get().GetCurrentFrameIndex()]) return;

    auto& context = (VulkanContext&)VulkanContext::Get();
    context.GetAllocator()->Unmap(m_AllocatedBuffers[GraphicsContext::Get().GetCurrentFrameIndex()].Allocation);
    m_bAreMapped[GraphicsContext::Get().GetCurrentFrameIndex()] = false;
}

void VulkanUniformBuffer::Update(void* data, const uint64_t size)
{
    auto& context                    = (VulkanContext&)VulkanContext::Get();
    const uint32_t currentFrameIndex = context.GetCurrentFrameIndex();

    void* mapped = RetrieveMapped();
    GNT_ASSERT(mapped, "Failed to map uniform buffer!");
    memcpy(mapped, data, size);

    Unmap();
    mapped = nullptr;
}

// STAGING

VulkanStagingBuffer::VulkanStagingBuffer(const uint64_t bufferSize) : m_Capacity(bufferSize)
{
    BufferUtils::CreateBuffer(EBufferUsageFlags::STAGING_BUFFER, m_Capacity, m_AllocatedBuffer, VMA_MEMORY_USAGE_CPU_ONLY);
}

void VulkanStagingBuffer::Destroy()
{
    BufferUtils::DestroyBuffer(m_AllocatedBuffer);
}

void VulkanStagingBuffer::SetData(const void* data, const uint64_t dataSize)
{
    if (dataSize > m_Capacity)
    {
        Resize(dataSize);
    }

    BufferUtils::CopyDataToBuffer(m_AllocatedBuffer, dataSize, data);
}

void VulkanStagingBuffer::Resize(const uint64_t newBufferSize)
{
    m_Capacity = newBufferSize;
    BufferUtils::DestroyBuffer(m_AllocatedBuffer);
    BufferUtils::CreateBuffer(EBufferUsageFlags::STAGING_BUFFER, m_Capacity, m_AllocatedBuffer, VMA_MEMORY_USAGE_CPU_ONLY);
}

}  // namespace Gauntlet
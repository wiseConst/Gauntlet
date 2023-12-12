#include "GauntletPCH.h"
#include "VulkanBuffer.h"

#include "VulkanContext.h"
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
    if (bufferUsage & EBufferUsageFlags::STORAGE_BUFFER) BufferUsageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    GNT_ASSERT(BufferUsageFlags != 0, "Unknown buffer usage flag!");
    return BufferUsageFlags;
}

void CreateBuffer(const EBufferUsage bufferUsage, const VkDeviceSize size, VulkanBuffer& outAllocatedBuffer, VmaMemoryUsage memoryUsage)
{
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.usage              = GauntletBufferUsageToVulkan(bufferUsage);
    bufferCreateInfo.size               = size;
    bufferCreateInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

    auto& context                 = (VulkanContext&)VulkanContext::Get();
    outAllocatedBuffer.Allocation = context.GetAllocator()->CreateBuffer(bufferCreateInfo, &outAllocatedBuffer.Buffer, memoryUsage);
}

void CopyBuffer(const VkBuffer& sourceBuffer, VkBuffer& destBuffer, const VkDeviceSize size)
{
    GRAPHICS_GUARD_LOCK;

    Ref<VulkanCommandBuffer> commandBuffer = MakeRef<VulkanCommandBuffer>(ECommandBufferType::COMMAND_BUFFER_TYPE_TRANSFER);
    commandBuffer->BeginRecording(true);

    VkBufferCopy copyRegion = {0, 0, size};
    commandBuffer->CopyBuffer(sourceBuffer, destBuffer, 1, &copyRegion);

    commandBuffer->EndRecording();
    commandBuffer->Submit();
}

void DestroyBuffer(VulkanBuffer& vulkanBuffer)
{
    GRAPHICS_GUARD_LOCK;
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    context.WaitDeviceOnFinish();
    context.GetAllocator()->DestroyBuffer(vulkanBuffer.Buffer, vulkanBuffer.Allocation);
    vulkanBuffer.Buffer     = VK_NULL_HANDLE;
    vulkanBuffer.Allocation = VK_NULL_HANDLE;
}

void CopyDataToBuffer(VulkanBuffer& vulkanBuffer, const VkDeviceSize dataSize, const void* data)
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(data, "Data you want to copy is not valid!");

    void* Mapped = context.GetAllocator()->Map(vulkanBuffer.Allocation);
    memcpy(Mapped, data, dataSize);
    context.GetAllocator()->Unmap(vulkanBuffer.Allocation);
}

}  // namespace BufferUtils

// VERTEX

VulkanVertexBuffer::VulkanVertexBuffer(BufferSpecification& bufferSpec) : m_BufferUsage(bufferSpec.Usage), m_VertexCount(bufferSpec.Count)
{
}

void VulkanVertexBuffer::SetData(const void* data, const size_t size)
{
    if (m_Handle.Buffer != VK_NULL_HANDLE)
    {
        BufferUtils::DestroyBuffer(m_Handle);
    }

    auto& stagingBuffer = Renderer::GetStorageData().UploadHeap;
    stagingBuffer->SetData(data, size);

    BufferUtils::CreateBuffer(m_BufferUsage, size, m_Handle, VMA_MEMORY_USAGE_GPU_ONLY);

    VkBuffer stagingBufferRaw = (VkBuffer)stagingBuffer->Get();
    GNT_ASSERT(stagingBufferRaw);
    BufferUtils::CopyBuffer(stagingBufferRaw, m_Handle.Buffer, size);
}

void VulkanVertexBuffer::Destroy()
{
    if (!m_Handle.Allocation)
    {
        LOG_TRACE("Unused vulkan buffer! No need to destroy, returning.");
        return;
    }

    BufferUtils::DestroyBuffer(m_Handle);
}

void VulkanVertexBuffer::SetStagedData(const Ref<StagingBuffer>& stagingBuffer, const uint64_t stagingBufferDataSize)
{
    // First call on this vertex buffer
    if (!m_Handle.Buffer)
    {
        BufferUtils::CreateBuffer(m_BufferUsage, stagingBufferDataSize, m_Handle, VMA_MEMORY_USAGE_GPU_ONLY);
    }

    // Special case if created buffer above has less size than we need to put into it.
    auto& Context                    = (VulkanContext&)VulkanContext::Get();
    VmaAllocationInfo AllocationInfo = {};
    Context.GetAllocator()->QueryAllocationInfo(AllocationInfo, m_Handle.Allocation);

    // If new buffer data size greater than our current buffer size, then recreate it
    if (stagingBufferDataSize > AllocationInfo.size)
    {
        BufferUtils::DestroyBuffer(m_Handle);
        BufferUtils::CreateBuffer(m_BufferUsage, stagingBufferDataSize, m_Handle, VMA_MEMORY_USAGE_GPU_ONLY);
    }

    VkBuffer stagingBufferRaw = (VkBuffer)stagingBuffer->Get();
    GNT_ASSERT(stagingBufferRaw);
    BufferUtils::CopyBuffer(stagingBufferRaw, m_Handle.Buffer, stagingBufferDataSize);
}

// INDEX

VulkanIndexBuffer::VulkanIndexBuffer(BufferSpecification& bufferSpec) : m_IndicesCount(bufferSpec.Count)
{
    GNT_ASSERT(bufferSpec.Data && bufferSpec.Size > 0);

    auto& stagingBuffer = Renderer::GetStorageData().UploadHeap;
    stagingBuffer->SetData(bufferSpec.Data, bufferSpec.Size);

    BufferUtils::CreateBuffer(bufferSpec.Usage | EBufferUsageFlags::TRANSFER_DST, bufferSpec.Size, m_Handle, VMA_MEMORY_USAGE_GPU_ONLY);

    VkBuffer stagingBufferRaw = (VkBuffer)stagingBuffer->Get();
    GNT_ASSERT(stagingBufferRaw);
    BufferUtils::CopyBuffer(stagingBufferRaw, m_Handle.Buffer, bufferSpec.Size);
}

void VulkanIndexBuffer::Destroy()
{
    BufferUtils::DestroyBuffer(m_Handle);
}

// UNIFORM

VulkanUniformBuffer::VulkanUniformBuffer(const uint64_t bufferSize) : m_Size(bufferSize), m_bIsMapped(false), m_bIsPersistent(false)
{
    BufferUtils::CreateBuffer(EBufferUsageFlags::UNIFORM_BUFFER, bufferSize, m_Handle, VMA_MEMORY_USAGE_CPU_TO_GPU);
}

void VulkanUniformBuffer::Destroy()
{
    auto& context = (VulkanContext&)VulkanContext::Get();

    if (m_bIsMapped)
    {
        context.GetAllocator()->Unmap(m_Handle.Allocation);
        m_bIsMapped = false;
    }

    BufferUtils::DestroyBuffer(m_Handle);
    m_Size          = 0;
    m_bIsPersistent = false;
}

void VulkanUniformBuffer::Map(bool bPersistent)
{
    if (m_bIsMapped) return;

    m_bIsPersistent = bPersistent;
    auto& context   = (VulkanContext&)VulkanContext::Get();

    context.GetAllocator()->Map(m_Handle.Allocation);
    m_bIsMapped = true;
}

void* VulkanUniformBuffer::RetrieveMapped()
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    if (m_bIsMapped)  // in case it's mapped, return void*
    {
        VmaAllocationInfo allocationInfo = {};
        context.GetAllocator()->QueryAllocationInfo(allocationInfo, m_Handle.Allocation);
        return allocationInfo.pMappedData;
    }

    m_bIsMapped = true;
    return context.GetAllocator()->Map(m_Handle.Allocation);  // in case we forgot to map, we instantly map it and return void*
}

void VulkanUniformBuffer::Unmap()
{
    if (!m_bIsMapped) return;

    auto& context = (VulkanContext&)VulkanContext::Get();
    context.GetAllocator()->Unmap(m_Handle.Allocation);
    m_bIsMapped = false;
}

void VulkanUniformBuffer::SetData(void* data, const uint64_t size)
{
    auto& context                    = (VulkanContext&)VulkanContext::Get();
    const uint32_t currentFrameIndex = context.GetCurrentFrameIndex();

    void* mapped = RetrieveMapped();
    GNT_ASSERT(mapped, "Failed to map uniform buffer!");
    memcpy(mapped, data, size);

    if (!m_bIsPersistent) Unmap();
    mapped = nullptr;
}

// STAGING

VulkanStagingBuffer::VulkanStagingBuffer(const uint64_t bufferSize) : m_Capacity(bufferSize)
{
    BufferUtils::CreateBuffer(EBufferUsageFlags::STAGING_BUFFER, m_Capacity, m_Handle, VMA_MEMORY_USAGE_CPU_ONLY);
}

void VulkanStagingBuffer::Destroy()
{
    BufferUtils::DestroyBuffer(m_Handle);
}

void VulkanStagingBuffer::SetData(const void* data, const uint64_t dataSize)
{
    if (dataSize > m_Capacity)
    {
        Resize(dataSize);
    }

    BufferUtils::CopyDataToBuffer(m_Handle, dataSize, data);
}

void VulkanStagingBuffer::Resize(const uint64_t newBufferSize)
{
    m_Capacity = newBufferSize;
    BufferUtils::DestroyBuffer(m_Handle);
    BufferUtils::CreateBuffer(EBufferUsageFlags::STAGING_BUFFER, m_Capacity, m_Handle, VMA_MEMORY_USAGE_CPU_ONLY);
}

VulkanStorageBuffer::VulkanStorageBuffer(const BufferSpecification& bufferSpec) : m_Specification(bufferSpec)
{
    if (m_Specification.Size == 0) return;

    if (m_Specification.Usage & EBufferUsageFlags::VERTEX_BUFFER)
        BufferUtils::CreateBuffer(m_Specification.Usage, m_Specification.Size, m_Handle, VMA_MEMORY_USAGE_GPU_ONLY);
    else
        BufferUtils::CreateBuffer(m_Specification.Usage, m_Specification.Size, m_Handle);
}

void VulkanStorageBuffer::Destroy()
{
    BufferUtils::DestroyBuffer(m_Handle);
}

void VulkanStorageBuffer::SetData(const void* data, const uint64_t dataSize)
{
    if (dataSize > m_Specification.Size)
    {
        m_Specification.Size = dataSize;
        if (m_Handle.Buffer) BufferUtils::DestroyBuffer(m_Handle);
        if (m_Specification.Usage & EBufferUsageFlags::VERTEX_BUFFER)
            BufferUtils::CreateBuffer(m_Specification.Usage, m_Specification.Size, m_Handle, VMA_MEMORY_USAGE_GPU_ONLY);
        else
            BufferUtils::CreateBuffer(m_Specification.Usage, m_Specification.Size, m_Handle);
    }

    auto& stagingBuffer = Renderer::GetStorageData().UploadHeap;
    stagingBuffer->SetData(data, dataSize);

    VkBuffer stagingBufferRaw = (VkBuffer)stagingBuffer->Get();
    GNT_ASSERT(stagingBufferRaw);
    BufferUtils::CopyBuffer(stagingBufferRaw, m_Handle.Buffer, dataSize);
}

}  // namespace Gauntlet
#pragma once

#include "Gauntlet/Core/Core.h"
#include "Gauntlet/Renderer/Buffer.h"

#include "VulkanAllocator.h"

namespace Gauntlet
{

// TODO: Should this class allow copying/moving itself??
struct VulkanAllocatedBuffer : public AllocatedBuffer /*: private Uncopyable, private Unmovable */
{
  public:
    VulkanAllocatedBuffer() : Allocation(VK_NULL_HANDLE), Buffer(VK_NULL_HANDLE) {}
    ~VulkanAllocatedBuffer() = default;

    VmaAllocation Allocation;  // Holds the state that the VMA library uses, like the memory that buffer was allocated from, and its size.
    VkBuffer Buffer;           // A handle to a GPU side Vulkan buffer
};

// VERTEX BUFFER

class VulkanVertexBuffer final : public VertexBuffer
{
  public:
    VulkanVertexBuffer() = delete;
    VulkanVertexBuffer(BufferInfo& bufferInfo);
    ~VulkanVertexBuffer() = default;

    FORCEINLINE const BufferLayout& GetLayout() const final override { return m_Layout; }
    FORCEINLINE void SetLayout(const BufferLayout& layout) final override { m_Layout = layout; }
    void SetData(const void* data, const size_t size) final override;

    FORCEINLINE uint64_t GetCount() const final override { return m_VertexCount; }
    void Destroy() final override;

    FORCEINLINE const void* Get() const final override { return m_AllocatedBuffer.Buffer; }
    FORCEINLINE void* Get() final override { return m_AllocatedBuffer.Buffer; }

    void SetStagedData(const VulkanAllocatedBuffer& stagingBuffer, const VkDeviceSize stagingBufferDataSize);

  private:
    VulkanAllocatedBuffer m_AllocatedBuffer;
    BufferLayout m_Layout;
    uint64_t m_VertexCount = 0;
};

// INDEX BUFFER

class VulkanIndexBuffer final : public IndexBuffer
{
  public:
    VulkanIndexBuffer() = delete;
    VulkanIndexBuffer(BufferInfo& bufferInfo);
    ~VulkanIndexBuffer() = default;

    uint64_t GetCount() const final override { return m_IndicesCount; }
    void Destroy() final override;

    FORCEINLINE const void* Get() const final override { return m_AllocatedBuffer.Buffer; }
    FORCEINLINE void* Get() final override { return m_AllocatedBuffer.Buffer; }

  private:
    VulkanAllocatedBuffer m_AllocatedBuffer;
    uint64_t m_IndicesCount = 0;
};

namespace BufferUtils
{
VkBufferUsageFlags GauntletBufferUsageToVulkan(const EBufferUsage bufferUsage);

void CreateBuffer(const EBufferUsage bufferUsage, const VkDeviceSize size, VulkanAllocatedBuffer& outAllocatedBuffer,
                  VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO);

void CopyBuffer(const VkBuffer& sourceBuffer, VkBuffer& destBuffer, const VkDeviceSize size);

void DestroyBuffer(VulkanAllocatedBuffer& buffer);

void CopyDataToBuffer(VulkanAllocatedBuffer& buffer, const VkDeviceSize dataSize, const void* data);

}  // namespace BufferUtils

}  // namespace Gauntlet
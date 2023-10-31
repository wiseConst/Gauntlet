#pragma once

#include "Gauntlet/Core/Core.h"
#include "Gauntlet/Renderer/Buffer.h"

#include "VulkanAllocator.h"

namespace Gauntlet
{

struct VulkanAllocatedBuffer final
{
    VulkanAllocatedBuffer()  = default;
    ~VulkanAllocatedBuffer() = default;

    VmaAllocation Allocation =
        VK_NULL_HANDLE;  // Holds the state that the VMA library uses, like the memory that buffer was allocated from, and its size.
    VkBuffer Buffer = VK_NULL_HANDLE;  // A handle to a GPU side Vulkan buffer
};

class VulkanStagingBuffer final : public StagingBuffer
{
  public:
    VulkanStagingBuffer(const uint64_t bufferSize);
    ~VulkanStagingBuffer() = default;

    void Destroy() final override;
    void SetData(const void* data, const uint64_t dataSize) final override;
    FORCEINLINE void* Get() const final override { return m_AllocatedBuffer.Buffer; }

  private:
    VulkanAllocatedBuffer m_AllocatedBuffer;
    VkDeviceSize m_Capacity = 0;

    void Resize(const uint64_t newBufferSize) final override;
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
    void SetStagedData(const Ref<StagingBuffer>& stagingBuffer, const uint64_t stagingBufferDataSize) final override;

    FORCEINLINE uint64_t GetCount() const final override { return m_VertexCount; }
    void Destroy() final override;

    FORCEINLINE const void* Get() const final override { return m_AllocatedBuffer.Buffer; }
    FORCEINLINE void* Get() final override { return m_AllocatedBuffer.Buffer; }

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

// UNIFORM BUFFER
class VulkanUniformBuffer final : public UniformBuffer
{
  public:
    VulkanUniformBuffer() = delete;
    VulkanUniformBuffer(const uint64_t bufferSize);
    ~VulkanUniformBuffer() = default;

    void Destroy() final override;

    void MapPersistent() final override;
    void* RetrieveMapped() final override;
    void Unmap() final override;

    void Update(void* data, const uint64_t size) final override;

    FORCEINLINE const auto& GetHandles() const { return m_AllocatedBuffers; }
    const uint64_t GetSize() const final override { return m_Size; }

  private:
    std::vector<VulkanAllocatedBuffer> m_AllocatedBuffers;  // Per-frame
    std::vector<bool> m_bAreMapped;                         // Per-frame for buffers
    VkDeviceSize m_Size = 0;
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
#pragma once

#include "Eclipse/Core/Core.h"
#include "Eclipse/Renderer/Buffer.h"

#include "VulkanAllocator.h"

namespace Eclipse
{

struct AllocatedBuffer /*: private Uncopyable, private Unmovable*/
{
  public:
    AllocatedBuffer() : Allocation(VK_NULL_HANDLE), Buffer(VK_NULL_HANDLE) {}
    ~AllocatedBuffer() = default;

    VmaAllocation Allocation;  // Holds the state that the VMA library uses, like the memory that buffer was allocated from, and its size.
    VkBuffer Buffer;           // A handle to a GPU side Vulkan buffer
};

// VERTEX BUFFER

class VulkanVertexBuffer final : public VertexBuffer
{
  public:
    VulkanVertexBuffer() = delete;
    VulkanVertexBuffer(BufferInfo& InBufferInfo);
    ~VulkanVertexBuffer() = default;

    FORCEINLINE const BufferLayout& GetLayout() const final override { return m_Layout; }
    FORCEINLINE void SetLayout(const BufferLayout& InLayout) final override { m_Layout = InLayout; }
    void SetData(const void* InData, const size_t InDataSize) final override;

    FORCEINLINE uint64_t GetCount() const final override { return m_VertexCount; }
    void Destroy() final override;

    FORCEINLINE const auto& Get() const { return m_AllocatedBuffer.Buffer; }
    FORCEINLINE auto& Get() { return m_AllocatedBuffer.Buffer; }

    void SetStagedData(const AllocatedBuffer& InStagingBuffer, const VkDeviceSize InBufferDataSize);

  private:
    AllocatedBuffer m_AllocatedBuffer;
    BufferLayout m_Layout;
    uint64_t m_VertexCount = 0;
};

// INDEX BUFFER

class VulkanIndexBuffer final : public IndexBuffer
{
  public:
    VulkanIndexBuffer() = delete;
    VulkanIndexBuffer(BufferInfo& InBufferInfo);
    ~VulkanIndexBuffer() = default;

    uint64_t GetCount() const final override { return m_IndicesCount; }
    void Destroy() final override;

    FORCEINLINE const auto& Get() const { return m_AllocatedBuffer.Buffer; }
    FORCEINLINE auto& Get() { return m_AllocatedBuffer.Buffer; }

  private:
    AllocatedBuffer m_AllocatedBuffer;
    uint64_t m_IndicesCount = 0;
};

namespace BufferUtils
{
VkBufferUsageFlags EclipseBufferUsageToVulkan(const EBufferUsage InBufferUsage);

void CreateBuffer(const EBufferUsage InBufferUsage, const VkDeviceSize InSize, AllocatedBuffer& InOutAllocatedBuffer,
                  VmaMemoryUsage InMemoryUsage = VMA_MEMORY_USAGE_AUTO);

void CopyBuffer(const VkBuffer& InSourceBuffer, VkBuffer& InDestBuffer, const VkDeviceSize InSize);

void DestroyBuffer(AllocatedBuffer& InBuffer);

void CopyDataToBuffer(AllocatedBuffer& InBuffer, const VkDeviceSize InDataSize, const void* InData);

}  // namespace BufferUtils

}  // namespace Eclipse
#pragma once

#include "Gauntlet/Core/Core.h"
#include "Gauntlet/Renderer/Buffer.h"

#include "VulkanAllocator.h"

namespace Gauntlet
{

struct VulkanBuffer final
{
    VulkanBuffer()  = default;
    ~VulkanBuffer() = default;

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

    FORCEINLINE void* Get() const final override { return m_Handle.Buffer; }
    FORCEINLINE size_t GetCapacity() const final override { return m_Capacity; }
    void Resize(const uint64_t newBufferSize) final override;

  private:
    VulkanBuffer m_Handle;
    VkDeviceSize m_Capacity = 0;
};

// VERTEX BUFFER

class VulkanVertexBuffer final : public VertexBuffer
{
  public:
    VulkanVertexBuffer() = delete;
    VulkanVertexBuffer(BufferSpecification& bufferSpec);
    ~VulkanVertexBuffer() = default;

    void SetData(const void* data, const size_t size) final override;
    void SetStagedData(const Ref<StagingBuffer>& stagingBuffer, const uint64_t stagingBufferDataSize) final override;

    FORCEINLINE uint64_t GetCount() const final override { return m_VertexCount; }
    void Destroy() final override;

    FORCEINLINE const void* Get() const final override { return m_Handle.Buffer; }

  private:
    VulkanBuffer m_Handle;
    uint64_t m_VertexCount     = 0;
    EBufferUsage m_BufferUsage = 0;
};

// INDEX BUFFER

class VulkanIndexBuffer final : public IndexBuffer
{
  public:
    VulkanIndexBuffer() = delete;
    VulkanIndexBuffer(BufferSpecification& bufferSpec);
    ~VulkanIndexBuffer() = default;

    uint64_t GetCount() const final override { return m_IndicesCount; }
    void Destroy() final override;

    FORCEINLINE const void* Get() const final override { return m_Handle.Buffer; }

  private:
    VulkanBuffer m_Handle;
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

    void Map(bool bPersistent = false) final override;
    void* RetrieveMapped() final override;
    void Unmap() final override;

    void SetData(void* data, const uint64_t size) final override;

    FORCEINLINE const auto& Get() const { return m_Handle.Buffer; }
    FORCEINLINE size_t GetSize() const final override { return m_Size; }

  private:
    VulkanBuffer m_Handle;
    VkDeviceSize m_Size  = 0;
    bool m_bIsMapped     = false;
    bool m_bIsPersistent = false;
};

// STORAGE BUFFER
class VulkanStorageBuffer final : public StorageBuffer
{
  public:
    VulkanStorageBuffer() = delete;
    VulkanStorageBuffer(const BufferSpecification& bufferSpec);
    ~VulkanStorageBuffer() = default;

    void Destroy() final override;
    void SetData(const void* data, const uint64_t dataSize) final override;

    FORCEINLINE void* Get() const final override { return m_Handle.Buffer; }
    FORCEINLINE size_t GetSize() const final override { return m_Specification.Size; }

  private:
    VulkanBuffer m_Handle;
    BufferSpecification m_Specification;
};

namespace BufferUtils
{
VkBufferUsageFlags GauntletBufferUsageToVulkan(const EBufferUsage bufferUsage);

void CreateBuffer(const EBufferUsage bufferUsage, const VkDeviceSize size, VulkanBuffer& outAllocatedBuffer,
                  VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO);

void CopyBuffer(const VkBuffer& sourceBuffer, VkBuffer& destBuffer, const VkDeviceSize size);

void DestroyBuffer(VulkanBuffer& buffer);

void CopyDataToBuffer(VulkanBuffer& buffer, const VkDeviceSize dataSize, const void* data);

}  // namespace BufferUtils

}  // namespace Gauntlet
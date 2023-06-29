#pragma once

#include "Eclipse/Renderer/Buffer.h"

namespace Eclipse
{

class VulkanVertexBuffer final : public VertexBuffer
{
  public:
    VulkanVertexBuffer() = delete;
    VulkanVertexBuffer(const BufferInfo& InBufferInfo);
    ~VulkanVertexBuffer();

    FORCEINLINE const BufferLayout& GetLayout() const final override { return m_Layout; }
    FORCEINLINE void SetLayout(const BufferLayout& InLayout) final override { m_Layout = InLayout; }

    FORCEINLINE uint64_t GetCount() const final override { return m_VertexCount; }
    void Destroy() final override;

  private:
    BufferLayout m_Layout;
    uint64_t m_VertexCount = 0;
};

class VulkanIndexBuffer final : public IndexBuffer
{
  public:
    VulkanIndexBuffer() = delete;
    VulkanIndexBuffer(const BufferInfo& InBufferInfo);
    ~VulkanIndexBuffer();

    uint64_t GetCount() const final override { return m_IndicesCount; }
    void Destroy() final override;

  private:
    uint64_t m_IndicesCount = 0;
};

namespace BufferUtils
{

}

}  // namespace Eclipse
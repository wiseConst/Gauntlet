#include "EclipsePCH.h"
#include "Buffer.h"

#include "Platform/Vulkan/VulkanBuffer.h"
#include "RendererAPI.h"

namespace Eclipse
{
// VERTEX

VertexBuffer::VertexBuffer(const BufferInfo& InBufferInfo) {}

VertexBuffer* VertexBuffer::Create(const BufferInfo& InBufferInfo)
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::None:
        {
            ELS_ASSERT(false, "RendererAPI is none!");
            return nullptr;
        }
        case RendererAPI::EAPI::Vulkan:
        {
            return new VulkanVertexBuffer(InBufferInfo);
        }
    }

    ELS_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

// INDEX

IndexBuffer::IndexBuffer(const BufferInfo& InBufferInfo) {}

IndexBuffer* IndexBuffer::Create(const BufferInfo& InBufferInfo)
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::None:
        {
            ELS_ASSERT(false, "RendererAPI is none!");
            return nullptr;
        }
        case RendererAPI::EAPI::Vulkan:
        {
            return new VulkanIndexBuffer(InBufferInfo);
        }
    }

    ELS_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}
}  // namespace Eclipse
#include "GauntletPCH.h"
#include "Buffer.h"

#include "Gauntlet/Platform/Vulkan/VulkanBuffer.h"
#include "RendererAPI.h"

namespace Gauntlet
{
// VERTEX

VertexBuffer::VertexBuffer(BufferInfo& InBufferInfo) {}

VertexBuffer* VertexBuffer::Create(BufferInfo& InBufferInfo)
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::None:
        {
            GNT_ASSERT(false, "RendererAPI is none!");
            return nullptr;
        }
        case RendererAPI::EAPI::Vulkan:
        {
            return new VulkanVertexBuffer(InBufferInfo);
        }
    }

    GNT_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

// INDEX

IndexBuffer::IndexBuffer(BufferInfo& InBufferInfo) {}

IndexBuffer* IndexBuffer::Create(BufferInfo& InBufferInfo)
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::None:
        {
            GNT_ASSERT(false, "RendererAPI is none!");
            return nullptr;
        }
        case RendererAPI::EAPI::Vulkan:
        {
            return new VulkanIndexBuffer(InBufferInfo);
        }
    }

    GNT_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}
Ref<UniformBuffer> UniformBuffer::Create(const uint64_t bufferSize)
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::None:
        {
            GNT_ASSERT(false, "RendererAPI is none!");
            return nullptr;
        }
        case RendererAPI::EAPI::Vulkan:
        {
            return MakeRef<VulkanUniformBuffer>(bufferSize);
        }
    }

    GNT_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

}  // namespace Gauntlet
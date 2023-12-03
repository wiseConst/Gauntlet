#include "GauntletPCH.h"
#include "Buffer.h"

#include "Gauntlet/Platform/Vulkan/VulkanBuffer.h"
#include "RendererAPI.h"

namespace Gauntlet
{
// VERTEX

Ref<VertexBuffer> VertexBuffer::Create(BufferSpecification& bufferSpec)
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
            return MakeRef<VulkanVertexBuffer>(bufferSpec);
        }
    }

    GNT_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

// INDEX

Ref<IndexBuffer> IndexBuffer::Create(BufferSpecification& bufferSpec)
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
            return MakeRef<VulkanIndexBuffer>(bufferSpec);
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

Ref<StagingBuffer> StagingBuffer::Create(const uint64_t bufferSize)
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
            return MakeRef<VulkanStagingBuffer>(bufferSize);
        }
    }

    GNT_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

Ref<StorageBuffer> StorageBuffer::Create(const BufferSpecification& bufferSpec)
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
            return MakeRef<VulkanStorageBuffer>(bufferSpec);
        }
    }

    GNT_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

}  // namespace Gauntlet
#pragma once

#include "Gauntlet/Core/Core.h"

namespace Gauntlet
{

enum class EShaderDataType : uint8_t
{
    None = 0,
    FLOAT,
    Vec2,
    Vec3,
    Vec4,
    Mat3,
    Mat4,
    Int,  // You can't pass it as shader attribute, only as uniform.
    Ivec2,
    Ivec3,
    Ivec4,
    Uvec4,
    Double,
    Bool
};

static uint32_t ShaderDataTypeSize(EShaderDataType type)
{
    switch (type)
    {
        case EShaderDataType::Bool: return 1;
        case EShaderDataType::Int: return 4;
        case EShaderDataType::FLOAT: return 4;
        case EShaderDataType::Double: return 8;

        case EShaderDataType::Ivec2:
        case EShaderDataType::Vec2: return 4 * 2;

        case EShaderDataType::Ivec3:
        case EShaderDataType::Vec3: return 4 * 3;

        case EShaderDataType::Ivec4:
        case EShaderDataType::Uvec4:
        case EShaderDataType::Vec4: return 4 * 4;

        case EShaderDataType::Mat3: return 4 * 3 * 3;
        case EShaderDataType::Mat4: return 4 * 4 * 4;
    }

    GNT_ASSERT(false, "Unknown shader data type!");
    return 0;
}

struct BufferElement
{
  public:
    BufferElement() = default;

    BufferElement(EShaderDataType type, const std::string_view& name) : Name(name), Type(type), Size(ShaderDataTypeSize(type)), Offset(0) {}

    ~BufferElement() = default;

    uint32_t GetComponentCount() const
    {
        switch (Type)
        {
            case EShaderDataType::Double: return 1;
            case EShaderDataType::Bool: return 1;
            case EShaderDataType::FLOAT: return 1;
            case EShaderDataType::Int: return 1;

            case EShaderDataType::Ivec2:
            case EShaderDataType::Vec2: return 2;

            case EShaderDataType::Ivec3:
            case EShaderDataType::Vec3: return 3;

            case EShaderDataType::Uvec4:
            case EShaderDataType::Ivec4:
            case EShaderDataType::Vec4: return 4;

            case EShaderDataType::Mat3: return 3 * 3;
            case EShaderDataType::Mat4: return 4 * 4;
        }

        GNT_ASSERT(false, "Unknown ShaderDataType!");
        return 0;
    }

  public:
    std::string_view Name;
    EShaderDataType Type = EShaderDataType::None;
    uint32_t Size        = 0;
    uint32_t Offset      = 0;
};

class BufferLayout final
{
  public:
    BufferLayout()  = default;
    ~BufferLayout() = default;

    BufferLayout(const std::initializer_list<BufferElement>& elements) : m_Elements(elements), m_Stride(0) { CalculateOffsetsAndStride(); }
    BufferLayout(const std::vector<BufferElement>& elements) : m_Elements(elements), m_Stride(0) { CalculateOffsetsAndStride(); }

    FORCEINLINE auto GetStride() const { return m_Stride; }
    FORCEINLINE const auto& GetElements() const { return m_Elements; }

    const auto& begin() const { return m_Elements.begin(); }
    const auto& end() const { return m_Elements.end(); }

    auto begin() { return m_Elements.begin(); }
    auto end() { return m_Elements.end(); }

  private:
    std::vector<BufferElement> m_Elements;
    uint32_t m_Stride = 0;

    void CalculateOffsetsAndStride()
    {
        for (auto& Element : m_Elements)
        {
            Element.Offset = m_Stride;
            m_Stride += Element.Size;
        }
    }
};

enum EBufferUsageFlags
{
    NONE           = BIT(0),
    STAGING_BUFFER = BIT(1),  // Means transfer source
    TRANSFER_DST   = BIT(2),
    UNIFORM_BUFFER = BIT(4),
    INDEX_BUFFER   = BIT(6),
    VERTEX_BUFFER  = BIT(7),
    STORAGE_BUFFER = BIT(8),
};

typedef uint32_t EBufferUsage;

// TODO: Refactor, create storage buffer
struct BufferSpecification final
{
    BufferSpecification() = default;

    BufferSpecification(EBufferUsage bufferUsage, const uint64_t size, const uint64_t count, void* data)
        : Usage(bufferUsage), Size(size), Count(count), Data(data)
    {
    }

    ~BufferSpecification() = default;

    EBufferUsage Usage = EBufferUsageFlags::NONE;
    size_t Size        = 0;  // Size in bytes
    size_t Count       = 0;

    void* Data = nullptr;
};

// VERTEX

class StagingBuffer;

class VertexBuffer : private Uncopyable, private Unmovable
{
  public:
    VertexBuffer()          = default;
    virtual ~VertexBuffer() = default;

    virtual void SetData(const void* data, const size_t dataSize)                                             = 0;
    virtual void SetStagedData(const Ref<StagingBuffer>& stagingBuffer, const uint64_t stagingBufferDataSize) = 0;

    virtual FORCEINLINE const void* Get() const = 0;

    virtual uint64_t GetCount() const = 0;
    virtual void Destroy()            = 0;

    static Ref<VertexBuffer> Create(BufferSpecification& bufferSpec);
};

// INDEX

class IndexBuffer : private Uncopyable, private Unmovable
{
  public:
    IndexBuffer()          = default;
    virtual ~IndexBuffer() = default;

    virtual FORCEINLINE const void* Get() const = 0;

    virtual uint64_t GetCount() const = 0;
    virtual void Destroy()            = 0;

    static Ref<IndexBuffer> Create(BufferSpecification& bufferSpec);
};

class UniformBuffer : private Uncopyable, private Unmovable
{
  public:
    UniformBuffer()          = default;
    virtual ~UniformBuffer() = default;

    virtual void Destroy() = 0;

    virtual void Map(bool bPersistent = false) = 0;
    virtual void* RetrieveMapped()             = 0;
    virtual void Unmap()                       = 0;

    virtual void SetData(void* data, const uint64_t size) = 0;
    virtual size_t GetSize() const                        = 0;

    static Ref<UniformBuffer> Create(const uint64_t bufferSize);
};

class StagingBuffer : private Uncopyable, private Unmovable
{
  public:
    StagingBuffer()          = default;
    virtual ~StagingBuffer() = default;

    virtual void Destroy()                                          = 0;
    virtual void SetData(const void* data, const uint64_t dataSize) = 0;

    virtual void Resize(const uint64_t newSize) = 0;
    virtual void* Get() const                   = 0;
    virtual size_t GetCapacity() const          = 0;

    static Ref<StagingBuffer> Create(const uint64_t bufferSize);
};

class StorageBuffer : private Uncopyable, private Unmovable
{
  public:
    StorageBuffer()          = default;
    virtual ~StorageBuffer() = default;

    virtual void Destroy()                                          = 0;
    virtual void SetData(const void* data, const uint64_t dataSize) = 0;

    virtual void* Get() const      = 0;
    virtual size_t GetSize() const = 0;

    static Ref<StorageBuffer> Create(const BufferSpecification& bufferSpec);
};

}  // namespace Gauntlet

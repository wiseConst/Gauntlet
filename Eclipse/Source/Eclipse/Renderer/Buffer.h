#pragma once

#include "Eclipse/Core/Core.h"

namespace Eclipse
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

static uint32_t ShaderDataTypeSize(EShaderDataType InType)
{
    switch (InType)
    {
        case EShaderDataType::FLOAT: return 4;
        case EShaderDataType::Vec2: return 4 * 2;
        case EShaderDataType::Vec3: return 4 * 3;
        case EShaderDataType::Vec4: return 4 * 4;
        case EShaderDataType::Mat3: return 4 * 3 * 3;
        case EShaderDataType::Mat4: return 4 * 4 * 4;
        case EShaderDataType::Int: return 4;
        case EShaderDataType::Ivec2: return 4 * 2;
        case EShaderDataType::Ivec3: return 4 * 3;
        case EShaderDataType::Ivec4: return 4 * 4;
        case EShaderDataType::Uvec4: return 4 * 4;
        case EShaderDataType::Double: return 8;
        case EShaderDataType::Bool: return 1;
    }

    ELS_ASSERT(false, "Unknown shader data type!");
    return 0;
}

struct BufferElement
{
  public:
    BufferElement() = default;

    BufferElement(EShaderDataType InType, const std::string_view& InName)
        : Name(InName), Type(InType), Size(ShaderDataTypeSize(InType)), Offset(0)
    {
    }

    ~BufferElement() = default;

    uint32_t GetComponentCount() const
    {
        switch (Type)
        {
            case EShaderDataType::FLOAT: return 1;
            case EShaderDataType::Vec2: return 2;
            case EShaderDataType::Vec3: return 3;
            case EShaderDataType::Vec4: return 4;
            case EShaderDataType::Mat3: return 3 * 3;
            case EShaderDataType::Mat4: return 4 * 4;
            case EShaderDataType::Int: return 1;
            case EShaderDataType::Ivec2: return 2;
            case EShaderDataType::Ivec3: return 3;
            case EShaderDataType::Ivec4: return 4;
            case EShaderDataType::Uvec4: return 4;
            case EShaderDataType::Double: return 1;
            case EShaderDataType::Bool: return 1;
        }

        ELS_ASSERT(false, "Unknown ShaderDataType!");
        return 0;
    }

  public:
    std::string_view Name;
    EShaderDataType Type;
    uint32_t Size;
    size_t Offset;
};

class BufferLayout final
{
  public:
    BufferLayout() : m_Stride(0) {}
    ~BufferLayout() = default;

    BufferLayout(const std::initializer_list<BufferElement>& InElements) : m_Elements(InElements), m_Stride(0)
    {
        CalculateOffsetsAndStride();
    }

    FORCEINLINE uint32_t GetStride() const { return m_Stride; }
    FORCEINLINE const std::vector<BufferElement>& GetElements() const { return m_Elements; }

    const auto& begin() const { return m_Elements.begin(); }
    const auto& end() const { return m_Elements.end(); }

    auto& begin() { return m_Elements.begin(); }
    auto& end() { return m_Elements.end(); }

  private:
    std::vector<BufferElement> m_Elements;
    uint32_t m_Stride;

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
};

typedef uint32_t EBufferUsage;

struct BufferInfo
{
  public:
    BufferInfo()
    {
        Usage = EBufferUsageFlags::NONE;
        Size  = 0;
        Count = 0;
        Data  = nullptr;
    }

    BufferInfo(EBufferUsage InBufferUsage, const uint64_t InSize, const uint64_t InCount, void* InData)
        : Usage(InBufferUsage), Size(InSize), Count(InCount), Data(InData)
    {
    }

    virtual ~BufferInfo() = default;

    BufferLayout Layout;
    EBufferUsage Usage;
    size_t Size;  // Size in bytes
    size_t Count;

    void* Data = nullptr;
};

// VERTEX

class VertexBuffer : private Uncopyable, private Unmovable
{
  public:
    VertexBuffer() = delete;
    VertexBuffer(BufferInfo& InBufferInfo);

    virtual ~VertexBuffer() = default;

    virtual const BufferLayout& GetLayout() const                     = 0;
    virtual void SetLayout(const BufferLayout& InLayout)              = 0;
    virtual void SetData(const void* InData, const size_t InDataSize) = 0;

    virtual uint64_t GetCount() const = 0;
    virtual void Destroy()            = 0;

    static VertexBuffer* Create(BufferInfo& InBufferInfo);
};

// INDEX

class IndexBuffer : private Uncopyable, private Unmovable
{
  public:
    IndexBuffer() = delete;
    IndexBuffer(BufferInfo& InBufferInfo);

    virtual ~IndexBuffer()            = default;
    virtual uint64_t GetCount() const = 0;

    virtual void Destroy() = 0;

    static IndexBuffer* Create(BufferInfo& InBufferInfo);
};
}  // namespace Eclipse

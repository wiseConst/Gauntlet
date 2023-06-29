#pragma once

#include "Eclipse/Core/Core.h"

namespace Eclipse
{

enum class EBufferUsageFlags
{
    VERTEX_BUFFER = 0,
    INDEX_BUFFER,
    UNIFORM_BUFFER,
    NONE
};

struct BufferInfo
{
  public:
    BufferInfo()
    {
        Usage = EBufferUsageFlags::NONE;
        Size = 0;
        Count = 0;
        Data = nullptr;
    }

    BufferInfo(EBufferUsageFlags usage, uint64_t size, uint64_t count, void* data) : Usage(usage), Size(size), Count(count), Data(data) {}

    virtual ~BufferInfo() = default;

    EBufferUsageFlags Usage;
    uint64_t Size;
    uint64_t Count;

    void* Data = nullptr;
};

enum class EShaderDataType : uint8_t
{
    None = 0,
    Float,
    Float2,
    Float3,
    Float4,
    Mat3,
    Mat4,
    Int,
    Int2,
    Int3,
    Int4,
    Bool
};

static uint32_t ShaderDataTypeSize(EShaderDataType InType)
{
    switch (InType)
    {
        case EShaderDataType::Float: return 4;
        case EShaderDataType::Float2: return 4 * 2;
        case EShaderDataType::Float3: return 4 * 3;
        case EShaderDataType::Float4: return 4 * 4;
        case EShaderDataType::Mat3: return 4 * 3 * 3;
        case EShaderDataType::Mat4: return 4 * 4 * 4;
        case EShaderDataType::Int: return 4;
        case EShaderDataType::Int2: return 4 * 2;
        case EShaderDataType::Int3: return 4 * 3;
        case EShaderDataType::Int4: return 4 * 4;
        case EShaderDataType::Bool: return 1;
    }

    ELS_ASSERT(false, "Unknown shader data type!");
    return 0;
}

struct BufferElement
{
  public:
    BufferElement() = default;

    BufferElement(EShaderDataType InType, const std::string& InName)
        : Name(InName), Type(InType), Size(ShaderDataTypeSize(InType)), Offset(0)
    {
    }

    uint32_t GetComponentCount() const
    {
        switch (Type)
        {
            case EShaderDataType::Float: return 1;
            case EShaderDataType::Float2: return 2;
            case EShaderDataType::Float3: return 3;
            case EShaderDataType::Float4: return 4;
            case EShaderDataType::Mat3: return 3 * 3;
            case EShaderDataType::Mat4: return 4 * 4;
            case EShaderDataType::Int: return 1;
            case EShaderDataType::Int2: return 2;
            case EShaderDataType::Int3: return 3;
            case EShaderDataType::Int4: return 4;
            case EShaderDataType::Bool: return 1;
        }

        ELS_ASSERT(false, "Unknown ShaderDataType!");
        return 0;
    }

  public:
    std::string Name;
    EShaderDataType Type;
    uint32_t Size;
    size_t Offset;
};

class BufferLayout final
{
  public:
    BufferLayout() {}

    BufferLayout(const std::initializer_list<BufferElement>& InElements) : m_Elements(InElements) { CalculateOffsetsAndStride(); }

    FORCEINLINE uint32_t GetStride() const { return m_Stride; }
    FORCEINLINE const std::vector<BufferElement>& GetElements() const { return m_Elements; }

    const auto& begin() const { return m_Elements.begin(); }
    const auto& end() const { return m_Elements.end(); }

    auto& begin() { return m_Elements.begin(); }
    auto& end() { return m_Elements.end(); }

  private:
    std::vector<BufferElement> m_Elements;
    uint32_t m_Stride = 0;

    void CalculateOffsetsAndStride()
    {
        m_Stride = 0;
        for (auto& element : m_Elements)
        {
            element.Offset = m_Stride;
            m_Stride += element.Size;
        }
    }
};

// VERTEX

class VertexBuffer : private Uncopyable, private Unmovable
{
  public:
    VertexBuffer() = delete;
    VertexBuffer(const BufferInfo& InBufferInfo);

    virtual ~VertexBuffer() = default;

    virtual const BufferLayout& GetLayout() const = 0;
    virtual void SetLayout(const BufferLayout& InLayout) = 0;

    virtual uint64_t GetCount() const = 0;
    virtual void Destroy() = 0;

    static VertexBuffer* Create(const BufferInfo& InBufferInfo);
};

// INDEX

class IndexBuffer : private Uncopyable, private Unmovable
{
  public:
    IndexBuffer() = delete;
    IndexBuffer(const BufferInfo& InBufferInfo);

    virtual ~IndexBuffer() = default;
    virtual uint64_t GetCount() const = 0;

    virtual void Destroy() = 0;

    static IndexBuffer* Create(const BufferInfo& InBufferInfo);
};
}  // namespace Eclipse

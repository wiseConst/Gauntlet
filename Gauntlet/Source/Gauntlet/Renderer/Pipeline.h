#pragma once

#include "Gauntlet/Core/Core.h"
#include "Shader.h"
#include "Buffer.h"

namespace Gauntlet
{

enum class ECullMode : uint8_t
{
    CULL_MODE_NONE = 0,
    CULL_MODE_FRONT,
    CULL_MODE_BACK,
    CULL_MODE_FRONT_AND_BACK
};

enum class EPolygonMode : uint8_t
{
    POLYGON_MODE_FILL = 0,
    POLYGON_MODE_LINE,
    POLYGON_MODE_POINT,
    POLYGON_MODE_FILL_RECTANGLE_NV
};

enum class EFrontFace : uint8_t
{
    FRONT_FACE_COUNTER_CLOCKWISE = 0,
    FRONT_FACE_CLOCKWISE
};

enum class ECompareOp : uint8_t
{
    COMPARE_OP_NEVER            = 0,
    COMPARE_OP_LESS             = 1,
    COMPARE_OP_EQUAL            = 2,
    COMPARE_OP_LESS_OR_EQUAL    = 3,
    COMPARE_OP_GREATER          = 4,
    COMPARE_OP_NOT_EQUAL        = 5,
    COMPARE_OP_GREATER_OR_EQUAL = 6,
    COMPARE_OP_ALWAYS           = 7,
};

enum class EPrimitiveTopology : uint8_t
{
    PRIMITIVE_TOPOLOGY_POINT_LIST = 0,
    PRIMITIVE_TOPOLOGY_LINE_LIST,
    PRIMITIVE_TOPOLOGY_LINE_STRIP,
    PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
    PRIMITIVE_TOPOLOGY_TRIANGLE_FAN
};

class Framebuffer;

struct PipelineSpecification
{
    std::string Name = "None";

    Ref<Shader> Shader                 = nullptr;
    Ref<Framebuffer> TargetFramebuffer = nullptr;
    BufferLayout Layout;

    bool PrimitiveRestartEnable          = false;
    EPrimitiveTopology PrimitiveTopology = EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    ECullMode CullMode       = ECullMode::CULL_MODE_NONE;
    float LineWidth          = 1.0f;
    EPolygonMode PolygonMode = EPolygonMode::POLYGON_MODE_FILL;
    EFrontFace FrontFace     = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;

    bool bDepthTest           = false;  // If we should do any z-culling at all
    bool bDepthWrite          = false;  // Allows the depth to be written.
    bool bDynamicPolygonMode  = false;
    bool bBlendEnable         = false;
    ECompareOp DepthCompareOp = ECompareOp::COMPARE_OP_NEVER;
};

class Pipeline : private Uncopyable, private Unmovable
{
  public:
    Pipeline()          = default;
    virtual ~Pipeline() = default;

    virtual void Destroy() = 0;

    static Ref<Pipeline> Create(const PipelineSpecification& pipelineSpec);

    FORCEINLINE virtual PipelineSpecification& GetSpecification() = 0;
};

}  // namespace Gauntlet
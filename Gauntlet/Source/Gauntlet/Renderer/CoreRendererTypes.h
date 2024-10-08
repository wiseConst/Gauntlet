#pragma once

#include "Gauntlet/Core/Math.h"
#include "Gauntlet/Core/Core.h"

namespace Gauntlet
{

// Vertices

struct QuadVertex
{
    glm::vec3 Position{0.0f};
    glm::vec4 Color{1.0f};
    glm::vec2 TexCoord{0.0f};
    float TextureId{0.0f};
    glm::vec3 Normal{0.0f};
};

struct MeshVertex
{
    glm::vec3 Position;
    glm::vec4 Color;
    glm::vec2 TexCoord;
    glm::vec3 Normal;
    glm::vec3 Tangent;
};

#define MAX_BONE_INFLUENCE 4

struct AnimatedVertex
{
    int32_t BoneIDs[MAX_BONE_INFLUENCE];  // bone indexes which will influence this vertex
    glm::vec3 Position;
    float Weights[MAX_BONE_INFLUENCE];  // weights from each bone
    glm::vec2 TexCoord;
    glm::vec3 Normal;
    glm::vec3 Tangent;
};

struct Vertex
{
    Vertex(const glm::vec3& position) : Position(position) {}

    glm::vec3 Position;
};

// Camera

struct UBCamera
{
    glm::mat4 Projection;
    glm::mat4 View;
    alignas(16) glm::vec3 Position;
};

// Lighting
static constexpr uint32_t s_MAX_POINT_LIGHTS = 16;
static constexpr uint32_t s_MAX_SPOT_LIGHTS  = 8;
static constexpr uint32_t s_MAX_DIR_LIGHTS   = 4;

struct PointLight
{
    glm::vec4 Position = glm::vec4(0.0f);
    glm::vec4 Color    = glm::vec4(0.0f);
    float Intensity    = 1.0f;
    float IsActive     = 0;
};

struct DirectionalLight
{
    glm::vec3 Color     = glm::vec3(0.0f);
    float Intensity     = 1.0f;
    glm::vec3 Direction = glm::vec3(0.0f);
    float CastShadows   = 0;
};

struct SpotLight
{
    glm::vec4 Position  = glm::vec4(0.0f);
    glm::vec4 Color     = glm::vec4(0.0f);
    glm::vec3 Direction = glm::vec3(0.0f);
    float CutOff        = 0.0f;
    float OuterCutOff   = 0.0f;

    float Intensity = 1.0f;
    float IsActive  = 0;
};

struct UBLighting
{
    DirectionalLight DirLights[s_MAX_DIR_LIGHTS];
    SpotLight SpotLights[s_MAX_SPOT_LIGHTS];
    PointLight PointLights[s_MAX_POINT_LIGHTS];

    float Gamma;
    float Exposure;
};

struct UBShadows
{
    glm::mat4 LightSpaceMatrix = glm::mat4(1.0f);
};

static constexpr uint32_t s_SSAO_KERNEL_SIZE = 16;
struct UBSSAO
{
    glm::mat4 CameraProjection  = glm::mat4(1.0f);
    glm::mat4 InvViewProjection = glm::mat4(1.0f);  // glm::lookAt returns inverse view matrix
    float Radius                = 0.5f;
    float Bias                  = 0.025f;
    int32_t Magnitude           = 1;
};

struct PBRMaterial
{
    glm::vec4 BaseColor = glm::vec4(1.0f);
    float Metallic      = 1.0f;
    float Roughness     = 1.0f;
    float padding0      = 0.0f;
    float padding1      = 0.0f;
};

// PUSH CONSTANTS

struct alignas(16) MeshPushConstants
{
    glm::mat4 TransformMatrix;
    glm::vec4 Data;  // It can be everything(in e.g. camera view position)
};

struct alignas(16) MatrixPushConstants
{
    glm::mat4 mat1;
    glm::mat4 mat2;
};

// USEFUL DEFINES

using UniformBufferPerFrame       = std::array<Ref<class UniformBuffer>, FRAMES_IN_FLIGHT>;
using FramebufferPerFrame         = std::array<Ref<class Framebuffer>, FRAMES_IN_FLIGHT>;
using RenderCommandBufferPerFrame = std::array<Ref<class CommandBuffer>, FRAMES_IN_FLIGHT>;

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

#if MESH_SHADING_TEST

// NVIDIA advice
struct Meshlet
{
    uint32_t vertices[64];
    uint8_t indices[126];  // max 42 primitives
    uint8_t triangleCount;
    uint8_t vertexCount;
};

#endif

}  // namespace Gauntlet
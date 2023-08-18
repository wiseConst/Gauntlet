#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Gauntlet
{
struct QuadVertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec4 Color;
    glm::vec2 TexCoord;
    float TextureId;
};

struct MeshVertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec4 Color;
    glm::vec2 TexCoord;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

struct Vertex
{
    Vertex(const glm::vec3& position) : Position(position) {}

    glm::vec3 Position;
};

struct CameraDataBuffer
{
    glm::mat4 Projection;
    glm::mat4 View;
    alignas(16) glm::vec3 Position;
};

static constexpr uint32_t s_MAX_POINT_LIGHTS = 4;

struct PhongModelBuffer
{
    glm::vec4 LightPosition;
    glm::vec4 LightColor;
    glm::vec4 AmbientSpecularShininessGamma;

    float Constant;
    float Linear;
    float Quadratic;
};

// PUSH CONSTANTS

struct alignas(16) MeshPushConstants
{
    glm::mat4 TransformMatrix;
    glm::vec4 Color;
};

}  // namespace Gauntlet
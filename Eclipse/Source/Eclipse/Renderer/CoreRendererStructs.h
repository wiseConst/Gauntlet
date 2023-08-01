#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Eclipse
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
    Vertex(const glm::vec3& InPosition) : Position(InPosition) {}

    glm::vec3 Position;
};

// From vkguide.dev unused currently
struct SceneDataBuffer
{
    glm::vec4 FogColor;      // w is for exponent
    glm::vec4 FogDistances;  // x for min, y for max, zw unused.
    glm::vec4 AmbientColor;
    glm::vec4 SunlightDirection;  // w for sun power
    glm::vec4 SunlightColor;
};

struct CameraDataBuffer
{
    glm::mat4 View;
    glm::mat4 Projection;
    alignas(16) glm::vec3 Position;
};

struct alignas(16) MeshPushConstants
{
    glm::mat4 RenderMatrix;
    glm::vec4 Color;
};

}  // namespace Eclipse
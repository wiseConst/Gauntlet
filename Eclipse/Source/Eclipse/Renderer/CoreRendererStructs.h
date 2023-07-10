#pragma once

#define GLM_FORCE_RADIANS
#define GLM_DEPTH_ZERO_TO_ONE
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

struct SceneDataBuffer
{
    glm::vec4 FogColor;      // w is for exponent
    glm::vec4 FogDistances;  // x for min, y for max, zw unused.
    glm::vec4 AmbientColor;
    glm::vec4 SunlightDirection;  // w for sun power
    glm::vec4 SunlightColor;
};

struct alignas(16) CameraDataBuffer
{
    glm::mat4 View;
    glm::mat4 Projection;
    glm::mat4 ViewProjection;
};

struct alignas(16) MeshPushConstants
{
    glm::mat4 RenderMatrix;
    glm::vec4 Color;
};

}  // namespace Eclipse
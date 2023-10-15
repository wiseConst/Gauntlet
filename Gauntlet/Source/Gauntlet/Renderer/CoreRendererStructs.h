#pragma once

#include "Gauntlet/Core/Math.h"

namespace Gauntlet
{

// Vertices

struct QuadVertex
{
    glm::vec3 Position{0.0f};
    glm::vec4 Color{1.0f};
    glm::vec2 TexCoord{0.0f};
    float TextureId{0.0f};
    glm::vec3 Normal{0.0f};  // vec3 normal for 2d rendering??
};

struct MeshVertex
{
    glm::vec3 Position;
    glm::vec4 Color;
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
static constexpr uint32_t s_MAX_DIR_LIGHTS   = 4;

// TODO: Add bool CastShadows;
struct PointLight
{
    glm::vec4 Position                 = glm::vec4(0.0f);
    glm::vec4 Color                    = glm::vec4(0.0f);
    glm::vec4 AmbientSpecularShininess = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);  // w reserved for CastShadows
    glm::vec4 CLQActive                = glm::vec4(1.0f, glm::vec3(0.0f));   // Attenuation: Constant Linear Quadratic IsActive?
};

struct DirectionalLight
{
    glm::vec4 Color                    = glm::vec4(0.0f);
    glm::vec4 Direction                = glm::vec4(0.0f);
    glm::vec4 AmbientSpecularShininess = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);  // w is bCastShadows
};

struct UBBlinnPhong
{
    DirectionalLight DirLight;
    PointLight PointLights[s_MAX_POINT_LIGHTS];

    alignas(16) float Gamma;
};

struct UBShadows
{
    glm::mat4 LightSpaceMatrix = glm::mat4(1.0f);
};

struct UBSSAO
{
    glm::mat4 CameraProjection = glm::mat4(1.0f);
    glm::vec4 Samples[16];
    glm::vec4 ViewportSizeNoiseFactor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
};

struct MaterialBuffer
{
    glm::vec4 BaseColor = glm::vec4(1.0f);
};

// PUSH CONSTANTS

struct alignas(16) MeshPushConstants
{
    glm::mat4 TransformMatrix;
    glm::vec4 Data;  // It can be everything(in e.g. camera view position)
};

struct alignas(16) LightPushConstants
{
    glm::mat4 Model;
    glm::mat4 LightSpaceProjection;
};

}  // namespace Gauntlet
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
    glm::vec4 Position                            = glm::vec4(0.0f);
    glm::vec4 Color                               = glm::vec4(0.0f);
    glm::vec4 AmbientSpecularShininessCastShadows = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
    glm::vec4 CLQActive                           = glm::vec4(1.0f, glm::vec3(0.0f));  // Attenuation: Constant Linear Quadratic IsActive?
};

struct DirectionalLight
{
    glm::vec4 Color                               = glm::vec4(0.0f);
    glm::vec4 Direction                           = glm::vec4(0.0f);
    glm::vec4 AmbientSpecularShininessCastShadows = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
};

struct SpotLight
{
    glm::vec4 Position  = glm::vec4(0.0f);
    glm::vec4 Color     = glm::vec4(0.0f);
    glm::vec3 Direction = glm::vec3(0.0f);
    float CutOff        = 0.0f;

    float Ambient   = 0.0f;
    float Specular  = 0.0f;
    float Shininess = 0.0f;
    int32_t Active  = 0;
};

struct UBBlinnPhong
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

struct UBSSAO
{
    glm::mat4 CameraProjection = glm::mat4(1.0f);
    glm::vec4 Samples[16];
    glm::vec4 ViewportSizeNoiseFactor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
    glm::vec4 RadiusBias              = glm::vec4(0.5f, 0.025f, 0.0f, 0.0f);
};

struct PBRMaterial
{
    glm::vec4 BaseColor = glm::vec4(1.0f);
    float Metallic      = 1.0f;
    float Roughness     = 1.0f;
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

struct alignas(16) MatPushConstants
{
    glm::mat4 mat1;
    glm::mat4 mat2;
};

}  // namespace Gauntlet
#pragma once

// Base components

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Gauntlet/Renderer/Camera/Camera.h"
#include "Gauntlet/Renderer/Mesh.h"
#include "Gauntlet/Renderer/Material.h"
#include "Gauntlet/Renderer/Texture.h"

namespace Gauntlet
{
// TODO: Do I actually emplace components in my GRECS???

struct TagComponent
{
    std::string Tag;

    TagComponent()                    = default;
    TagComponent(const TagComponent&) = default;
    TagComponent(const std::string& InTag) : Tag(InTag) {}
};

struct TransformComponent
{
    bool Padlock{false};
    glm::vec3 Translation{0.0f};
    glm::vec3 Rotation{0.0f};
    glm::vec3 Scale{1.0f};

    TransformComponent()                          = default;
    TransformComponent(const TransformComponent&) = default;
    TransformComponent(const glm::vec3& InTranslation) : Translation(InTranslation) {}

    glm::mat4 GetTransform() const
    {
        const float RotationX    = Rotation.x;
        const float RotationY    = Rotation.y;
        const float RotationZ    = Rotation.z;
        const glm::mat4 Rotation = glm::rotate(glm::mat4(1.0f), glm::radians(RotationX), glm::vec3(1, 0, 0)) *
                                   glm::rotate(glm::mat4(1.0f), glm::radians(RotationY), glm::vec3(0, 1, 0)) *
                                   glm::rotate(glm::mat4(1.0f), glm::radians(RotationZ), glm::vec3(0, 0, 1));

        // If you don't understand order look at this: https://learnopengl.com/Getting-started/Transformations
        return glm::translate(glm::mat4(1.0f), Translation) * Rotation * glm::scale(glm::mat4(1.0f), Scale);
    }

    operator glm::mat4() const { return GetTransform(); }
};

struct SpriteRendererComponent
{
    glm::vec4 Color{1.0f};

    SpriteRendererComponent()                               = default;
    SpriteRendererComponent(const SpriteRendererComponent&) = default;
    SpriteRendererComponent(const glm::vec4& InColor) : Color(InColor) {}
};

struct CameraComponent
{
    Ref<Gauntlet::Camera> Camera;
    bool bIsPrimary = true;

    CameraComponent()                       = default;
    CameraComponent(const CameraComponent&) = default;
    CameraComponent(const Ref<Gauntlet::Camera>& InCamera) : Camera(InCamera) {}
};

struct MeshComponent
{
    Ref<Gauntlet::Mesh> Mesh;

    MeshComponent()                     = default;
    MeshComponent(const MeshComponent&) = default;
    MeshComponent(const Ref<Gauntlet::Mesh>& InMesh) : Mesh(InMesh) {}
};

struct PointLightComponent
{
    glm::vec3 Color;
    glm::vec3 AmbientSpecularShininess;
    glm::vec3 CLQ;  // Constant Linear Quadratic

    PointLightComponent()                           = default;
    PointLightComponent(const PointLightComponent&) = default;
};

struct DirectionalLightComponent
{
    glm::vec3 Color;
    glm::vec3 Direction;
    glm::vec3 AmbientSpecularShininess;

    DirectionalLightComponent()                                 = default;
    DirectionalLightComponent(const DirectionalLightComponent&) = default;
};

}  // namespace Gauntlet
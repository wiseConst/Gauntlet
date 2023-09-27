#pragma once

// Base components

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Gauntlet/Core/UUID.h"
#include "Gauntlet/Renderer/Camera/Camera.h"
#include "Gauntlet/Renderer/Mesh.h"
#include "Gauntlet/Renderer/Material.h"
#include "Gauntlet/Renderer/Texture.h"

namespace Gauntlet
{

struct IDComponent
{
    UUID ID;

    IDComponent()                   = default;
    IDComponent(const IDComponent&) = default;
    IDComponent(const UUID& uuid) : ID(uuid) {}
};

struct TagComponent
{
    std::string Tag{"None"};

    TagComponent()                    = default;
    TagComponent(const TagComponent&) = default;
    TagComponent(const std::string& tag) : Tag(tag) {}
};

struct TransformComponent
{
    glm::vec3 Translation{0.0f};
    glm::vec3 Rotation{0.0f};
    glm::vec3 Scale{1.0f};
    bool Padlock{false};

    TransformComponent()                          = default;
    TransformComponent(const TransformComponent&) = default;
    TransformComponent(const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale)
        : Translation(translation), Rotation(rotation), Scale(scale)
    {
    }

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
    SpriteRendererComponent(const glm::vec4& color) : Color(color) {}
};

struct CameraComponent
{
    Ref<Gauntlet::Camera> Camera{nullptr};
    bool bIsActive = true;

    CameraComponent()                       = default;
    CameraComponent(const CameraComponent&) = default;
    CameraComponent(const Ref<Gauntlet::Camera>& camera) : Camera(camera) {}
};

struct MeshComponent
{
    Ref<Gauntlet::Mesh> Mesh{nullptr};

    MeshComponent()                     = default;
    MeshComponent(const MeshComponent&) = default;
    MeshComponent(const Ref<Gauntlet::Mesh>& mesh) : Mesh(mesh) {}
};

struct PointLightComponent
{
    glm::vec3 Color{0.0f};
    glm::vec3 AmbientSpecularShininess{glm::vec2(0.0f), 1.0f};
    glm::vec3 CLQ{1.0f, 0.0f, 0.0f};  // Constant Linear Quadratic

    PointLightComponent()                           = default;
    PointLightComponent(const PointLightComponent&) = default;
};

struct DirectionalLightComponent
{
    glm::vec3 Color{0.0f};
    glm::vec3 AmbientSpecularShininess{glm::vec2(0.0f), 1.0f};
    bool bCastShadows = false;

    DirectionalLightComponent()                                 = default;
    DirectionalLightComponent(const DirectionalLightComponent&) = default;
};

}  // namespace Gauntlet
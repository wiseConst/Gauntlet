#pragma once

#include "Eclipse/Core/Core.h"
#include "Eclipse/Core/Input.h"

#include "Eclipse/Event/Event.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Eclipse/Core/Application.h"
#include "Eclipse/Core/Window.h"

namespace Eclipse
{

class Camera : private Uncopyable, private Unmovable
{

  public:
    virtual void OnUpdate(const float DeltaTime) = 0;
    virtual void OnEvent(Event& InEvent)         = 0;

    FORCEINLINE const auto& GetPosition() const { return m_Position; }
    FORCEINLINE const auto& GetViewMatrix() const { return m_ViewMatrix; }
    FORCEINLINE const auto GetViewProjectionMatrix() const { return m_ProjectionMatrix * m_ViewMatrix; }
    FORCEINLINE const auto& GetProjectionMatrix() const { return m_ProjectionMatrix; }
    FORCEINLINE const auto& GetRotation() const { return m_Rotation; }
    FORCEINLINE const auto GetAspectRatio() const { return m_AspectRatio; }

    FORCEINLINE void SetPosition(const glm::vec3& InPosition)
    {
        m_Position = InPosition;
        RecalculateViewMatrix();
    }

    FORCEINLINE void SetRotation(const glm::vec3& InRotation)
    {
        if (InRotation.z > 180.0f)
            m_Rotation -= 360.0f;
        else if (InRotation.z <= -180.0f)
            m_Rotation += 360.0f;

        RecalculateViewMatrix();
    }

  protected:
    glm::mat4 m_ProjectionMatrix;
    glm::mat4 m_ViewMatrix;  // Currently unused

    glm::vec3 m_Position = {0.f, 0.f, 0.f};
    glm::vec3 m_Rotation = {0.f, 0.f, 0.f};  // Z axis in degrees

    float m_AspectRatio;
    float m_CameraTranslationSpeed = 1.0f;
    float m_CameraRotationSpeed    = 180.0f;

    Camera(const float InAspectRatio = 0.0f)
        : m_AspectRatio(InAspectRatio), m_ProjectionMatrix(glm::mat4(1.f)), m_ViewMatrix(glm::mat4(1.f))
    {
        if (InAspectRatio == 0.0f)
        {
            m_AspectRatio = Application::Get().GetWindow()->GetAspectRatio();
        }
    }
    virtual ~Camera() = default;

    FORCEINLINE void RecalculateViewMatrix()
    {
        glm::mat4 TransformMatrix = glm::translate(glm::mat4(1.0f), m_Position) *
                                    glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

        m_ViewMatrix = glm::inverse(TransformMatrix);
    };
};

}  // namespace Eclipse
#pragma once

#include "Camera.h"

namespace Eclipse
{

class PerspectiveCamera final : public Camera
{
  public:
    PerspectiveCamera(const float InAspectRatio) : Camera(InAspectRatio) { SetProjection(0.01f, 1000.0f); }

    ~PerspectiveCamera() = default;

    FORCEINLINE const auto GetFOV() const { return m_FOV; }

    void OnUpdate(const float DeltaTime) final override
    {
        if (Input::IsKeyPressed(ELS_KEY_W))
            m_Position.y += m_CameraTranslationSpeed * DeltaTime;
        else if (Input::IsKeyPressed(ELS_KEY_S))
            m_Position.y -= m_CameraTranslationSpeed * DeltaTime;

        if (Input::IsKeyPressed(ELS_KEY_A))
            m_Position.x -= m_CameraTranslationSpeed * DeltaTime;
        else if (Input::IsKeyPressed(ELS_KEY_D))
            m_Position.x += m_CameraTranslationSpeed * DeltaTime;

        if (Input::IsKeyPressed(ELS_KEY_Q)) m_Rotation.z -= m_CameraRotationSpeed * DeltaTime;
        if (Input::IsKeyPressed(ELS_KEY_E)) m_Rotation.z += m_CameraRotationSpeed * DeltaTime;

        SetRotation(m_Rotation);

        m_CameraTranslationSpeed = m_FOV;
        SetPosition(m_Position);
    }

    void SetProjection(const float zNear = 0.0f, const float zFar = 1.0f)
    {
        m_ProjectionMatrix = glm::perspective(glm::radians(m_FOV), m_AspectRatio, zNear, zFar);
        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }

    void OnEvent(Event& InEvent) final override
    {
        if (InEvent.GetType() == EEventType::WindowResizeEvent)
        {
            OnWindowResized((WindowResizeEvent&)InEvent);
        }

        if (InEvent.GetType() == EEventType::MouseScrolledEvent)
        {
            OnMouseScrolled((MouseScrolledEvent&)InEvent);
        }
    }

  private:
    float m_FOV = 90.0f;

    void OnWindowResized(WindowResizeEvent& InEvent)
    {
        m_AspectRatio = InEvent.GetAspectRatio();
        SetProjection(0.01f, 1000.0f);
    }

    void OnMouseScrolled(MouseScrolledEvent& InEvent)
    {
        m_FOV += InEvent.GetOffsetY() / 2.0f;
        if (m_FOV < 0.0f) m_FOV = 0.0f;

        SetProjection(0.01f, 1000.0f);
    }
};

}  // namespace Eclipse
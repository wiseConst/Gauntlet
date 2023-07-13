#pragma once

#include "Camera.h"

#include "Eclipse/Event/Event.h"
#include "Eclipse/Event/WindowEvent.h"
#include "Eclipse/Event/MouseEvent.h"

namespace Eclipse
{

class OrthographicCamera final : public Camera
{
  public:
    OrthographicCamera(const float InAspectRatio = 0.0f, const float InCameraWidth = 8.0f)
        : Camera(InAspectRatio), m_CameraWidth(InCameraWidth)
    {
        ELS_ASSERT(InCameraWidth > 0.0f, "Camera width can only be positive!");
        SetProjection(m_AspectRatio * -m_CameraWidth, m_AspectRatio * m_CameraWidth, -m_CameraWidth, m_CameraWidth, -m_CameraWidth,
                      m_CameraWidth);
        m_ZoomLevel = m_CameraWidth;
    }
    ~OrthographicCamera() = default;

    FORCEINLINE const auto& GetZoomLevel() const { return m_ZoomLevel; }

    void OnUpdate(const float DeltaTime) final override
    {
        if (Input::IsKeyPressed(ELS_KEY_S)) m_Position.y -= m_CameraTranslationSpeed * DeltaTime;
        if (Input::IsKeyPressed(ELS_KEY_W)) m_Position.y += m_CameraTranslationSpeed * DeltaTime;

        if (Input::IsKeyPressed(ELS_KEY_A)) m_Position.x -= m_CameraTranslationSpeed * DeltaTime;
        if (Input::IsKeyPressed(ELS_KEY_D)) m_Position.x += m_CameraTranslationSpeed * DeltaTime;

        if (Input::IsKeyPressed(ELS_KEY_Q)) m_Rotation.z += m_CameraRotationSpeed * DeltaTime;
        if (Input::IsKeyPressed(ELS_KEY_E)) m_Rotation.z -= m_CameraRotationSpeed * DeltaTime;

        SetRotation(m_Rotation);

        m_CameraTranslationSpeed = m_ZoomLevel;
        SetPosition(m_Position);
    }

    void OnEvent(Event& InEvent) final override
    {
        if (InEvent.GetType() == EEventType::WindowResizeEvent)
        {
            OnWindowResized((WindowResizeEvent&)InEvent);
        }

        /*if (InEvent.GetType() == EEventType::MouseScrolledEvent)
        {
            OnMouseScrolled((MouseScrolledEvent&)InEvent);
        }*/
    }

    FORCEINLINE void SetProjection(const float left, const float right, const float bottom, const float top, const float zNear,
                                   const float zFar)
    {
        m_ProjectionMatrix = glm::ortho(left, right, bottom, top, zNear, zFar);
    }

    FORCEINLINE static Ref<OrthographicCamera> Create(const float InAspectRatio = 0.0f, const float InCameraWidth = 8.0f)
    {
        return Ref<OrthographicCamera>(new OrthographicCamera(InAspectRatio, InCameraWidth));
    }

    FORCEINLINE const float GetWidthBorder() const { return m_ZoomLevel * m_AspectRatio; }
    FORCEINLINE const float GetHeightBorder() const { return m_ZoomLevel; }

  private:
    float m_ZoomLevel         = 1.5f;
    const float m_CameraWidth = 8.0f;

    void OnWindowResized(WindowResizeEvent& InEvent)
    {
        m_AspectRatio = InEvent.GetAspectRatio();
        SetProjection(m_AspectRatio * -m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);
    }

    void OnMouseScrolled(MouseScrolledEvent& InEvent)
    {
        m_ZoomLevel -= InEvent.GetOffsetY() * 0.25f;
        m_ZoomLevel = std::max(m_ZoomLevel, 0.25f);

        SetProjection(m_AspectRatio * -m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);
    }
};

}  // namespace Eclipse
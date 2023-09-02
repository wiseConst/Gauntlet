#pragma once

#include "Camera.h"

#include "Gauntlet/Event/Event.h"
#include "Gauntlet/Event/WindowEvent.h"
#include "Gauntlet/Event/MouseEvent.h"

namespace Gauntlet
{

class OrthographicCamera final : public Camera
{
  public:
    OrthographicCamera(const float aspectRatio = 0.0f, const float cameraWidth = 10.0f) : Camera(aspectRatio), m_CameraWidth(cameraWidth)
    {
        GNT_ASSERT(aspectRatio >= 0.0f && cameraWidth > 0.0f, "Camera width && aspect ratio can only be positive!");
        SetProjection(m_AspectRatio * -m_CameraWidth, m_AspectRatio * m_CameraWidth, -m_CameraWidth, m_CameraWidth, -m_CameraWidth,
                      m_CameraWidth);
        m_ZoomLevel = m_CameraWidth;
    }
    ~OrthographicCamera() = default;

    FORCEINLINE const auto& GetZoomLevel() const { return m_ZoomLevel; }

    void OnUpdate(const float deltaTime) final override
    {
        if (Input::IsKeyPressed(KeyCode::KEY_S)) m_Position.y -= m_CameraTranslationSpeed * deltaTime;
        if (Input::IsKeyPressed(KeyCode::KEY_W)) m_Position.y += m_CameraTranslationSpeed * deltaTime;

        if (Input::IsKeyPressed(KeyCode::KEY_A)) m_Position.x -= m_CameraTranslationSpeed * deltaTime;
        if (Input::IsKeyPressed(KeyCode::KEY_D)) m_Position.x += m_CameraTranslationSpeed * deltaTime;

        if (Input::IsKeyPressed(KeyCode::KEY_Q)) m_Rotation.z += m_CameraRotationSpeed * deltaTime;
        if (Input::IsKeyPressed(KeyCode::KEY_E)) m_Rotation.z -= m_CameraRotationSpeed * deltaTime;

        SetRotation(m_Rotation);

        m_CameraTranslationSpeed = m_ZoomLevel;
        SetPosition(m_Position);
    }

    void OnEvent(Event& event) final override
    {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<WindowResizeEvent>(BIND_FN(OrthographicCamera::OnWindowResized));
        dispatcher.Dispatch<MouseScrolledEvent>(BIND_FN(OrthographicCamera::OnMouseScrolled));
    }

    FORCEINLINE void SetProjection(const float left, const float right, const float bottom, const float top, const float zNear,
                                   const float zFar)
    {
        m_ProjectionMatrix = glm::ortho(left, right, bottom, top, zNear, zFar);
    }

    FORCEINLINE static Ref<OrthographicCamera> Create(const float aspectRatio = 0.0f, const float cameraWidth = 8.0f)
    {
        return Ref<OrthographicCamera>(new OrthographicCamera(aspectRatio, cameraWidth));
    }

    FORCEINLINE const float GetWidthBorder() const { return m_ZoomLevel * m_AspectRatio; }
    FORCEINLINE const float GetHeightBorder() const { return m_ZoomLevel; }

  private:
    float m_ZoomLevel         = 1.5f;
    const float m_CameraWidth = 10.0f;

    void OnWindowResized(WindowResizeEvent& event)
    {
        m_AspectRatio = event.GetAspectRatio();
        SetProjection(m_AspectRatio * -m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);
    }

    void OnMouseScrolled(MouseScrolledEvent& event)
    {
        m_ZoomLevel -= event.GetOffsetY() * 0.25f;
        m_ZoomLevel = std::max(m_ZoomLevel, 0.25f);

        SetProjection(m_AspectRatio * -m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);
    }

    void RecalculateViewMatrix() final override
    {
        glm::mat4 TransformMatrix = glm::translate(glm::mat4(1.0f), m_Position) *
                                    glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

        m_ViewMatrix = glm::inverse(TransformMatrix);
    };
};

}  // namespace Gauntlet
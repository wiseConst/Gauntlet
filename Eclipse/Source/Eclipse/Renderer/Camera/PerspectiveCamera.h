#pragma once

#include "Camera.h"

// From https://learnopengl.com/Getting-started/Camera

namespace Eclipse
{

class PerspectiveCamera : public Camera
{
  public:
    // If AspectRatio eq 0.0f it's gonna be calculated based on window size.
    PerspectiveCamera(const float InAspectRatio = 0.0f, const float InFOV = 90.0f) : Camera(InAspectRatio), m_FOV(InFOV)
    {
        SetProjection(0.1f, 1000.0f);
        m_CameraTranslationSpeed = 2.0f;
    }
    ~PerspectiveCamera() = default;

    FORCEINLINE static Ref<PerspectiveCamera> Create(const float InAspectRatio = 0.0f, const float InFOV = 90.0f)
    {
        return Ref<PerspectiveCamera>(new PerspectiveCamera(InAspectRatio, InFOV));
    }

    FORCEINLINE const auto GetFOV() const { return m_FOV; }

    void OnUpdate(const float DeltaTime) override
    {
        if (Input::IsKeyPressed(ELS_KEY_W))
            m_Position += m_Front * m_CameraTranslationSpeed * DeltaTime;
        else if (Input::IsKeyPressed(ELS_KEY_S))
            m_Position -= m_Front * m_CameraTranslationSpeed * DeltaTime;

        if (Input::IsKeyPressed(ELS_KEY_A))
            m_Position -= m_Right * m_CameraTranslationSpeed * DeltaTime;
        else if (Input::IsKeyPressed(ELS_KEY_D))
            m_Position += m_Right * m_CameraTranslationSpeed * DeltaTime;

        if (Input::IsKeyPressed(ELS_KEY_SPACE))
            m_Position += m_Up * m_CameraTranslationSpeed * DeltaTime;
        else if (Input::IsKeyPressed(ELS_KEY_C))
            m_Position -= m_Up * m_CameraTranslationSpeed * DeltaTime;

        if (Input::IsKeyPressed(ELS_KEY_LEFT_SHIFT))
            m_CameraTranslationSpeed = 50.0f;
        else
            m_CameraTranslationSpeed = 2.0f;

        SetPosition(m_Position);
    }

    void SetProjection(const float zNear = 0.01f, const float zFar = 1000.0f)
    {
        m_ProjectionMatrix = glm::perspective(glm::radians(m_FOV), m_AspectRatio, zNear, zFar);
    }

    void OnEvent(Event& InEvent) override
    {
        EventDispatcher dispatcher(InEvent);
        dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(PerspectiveCamera::OnWindowResized));
        dispatcher.Dispatch<MouseScrolledEvent>(BIND_EVENT_FN(PerspectiveCamera::OnMouseScrolled));
        dispatcher.Dispatch<MouseMovedEvent>(BIND_EVENT_FN(PerspectiveCamera::OnMouseMoved));
    }

    FORCEINLINE void SetSensitivity(const float InSensitivity = 0.1f) { m_Sensitivity = InSensitivity; }

  private:
    float m_FOV         = 90.0f;
    float m_Sensitivity = 0.1f;

    glm::vec3 m_Front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 m_Up    = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 m_Right = glm::vec3(1.0f, 0.0f, 0.0f);

    float m_Yaw   = -90.0f;
    float m_Pitch = 0.0f;

    float m_LastMouseX{0.0f};
    float m_LastMouseY{0.0f};

    bool m_FirstMouse{true};

    void OnWindowResized(WindowResizeEvent& InEvent)
    {
        m_AspectRatio = InEvent.GetAspectRatio();
        SetProjection();
    }

    void OnMouseScrolled(MouseScrolledEvent& InEvent)
    {
        m_FOV -= InEvent.GetOffsetY();
        m_FOV = std::clamp(m_FOV, 45.0f, 110.0f);

        SetProjection();
    }

    void OnMouseMoved(MouseMovedEvent& InEvent)
    {
        if (m_FirstMouse)
        {
            m_LastMouseX = InEvent.GetOffsetX();
            m_LastMouseY = InEvent.GetOffsetY();
            m_FirstMouse = false;
        }

        const float DiffX = InEvent.GetOffsetX() - m_LastMouseX;
        const float DiffY = m_LastMouseY - InEvent.GetOffsetY();

        m_LastMouseX = static_cast<float>(InEvent.GetOffsetX());
        m_LastMouseY = static_cast<float>(InEvent.GetOffsetY());

        if (!Input::IsMouseButtonPressed(ELS_MOUSE_BUTTON_1)) return;

        m_Yaw += DiffX * m_Sensitivity;

        m_Pitch += DiffY * m_Sensitivity;
        m_Pitch = std::clamp(m_Pitch, -89.0f, 89.0f);

        glm::vec3 CameraDirection = glm::vec3(1.0f);
        CameraDirection.x         = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
        CameraDirection.y         = sin(glm::radians(m_Pitch));
        CameraDirection.z         = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
        m_Front                   = glm::normalize(CameraDirection);

        m_Right = glm::cross(m_Front, m_Up);
        RecalculateViewMatrix();
    }

    void RecalculateViewMatrix() override { m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_Front, m_Up); }
};

}  // namespace Eclipse
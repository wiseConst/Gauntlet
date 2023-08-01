#pragma once

#include <Eclipse.h>

class EditorCamera final : public Eclipse::PerspectiveCamera
{
  public:
    // If AspectRatio eq 0.0f it's gonna be calculated based on window size.
    EditorCamera(const float InAspectRatio = 0.0f, const float InFOV = 90.0f) : Eclipse::PerspectiveCamera(InAspectRatio, InFOV)
    {
        m_ViewportWidth  = static_cast<float>(Eclipse::Application::Get().GetWindow()->GetWidth());
        m_ViewportHeight = static_cast<float>(Eclipse::Application::Get().GetWindow()->GetHeight());
        SetSensitivity(0.2f);
        // SetProjection();
    }

    ~EditorCamera() = default;

    FORCEINLINE static Eclipse::Ref<EditorCamera> Create(const float InAspectRatio = 0.0f, const float InFOV = 90.0f)
    {
        return Eclipse::Ref<EditorCamera>(new EditorCamera(InAspectRatio, InFOV));
    }

    // void OnUpdate(const float DeltaTime) final override {}

    // void OnEvent(Eclipse::Event& InEvent) final override {}

    void Resize(uint32_t InWidth, uint32_t InHeight)
    {
        LOG_INFO("Editor Camera Viewport Resize!");

        ELS_ASSERT(InHeight != 0, "Zero division!");
        m_ViewportWidth  = static_cast<float>(InWidth);
        m_ViewportHeight = static_cast<float>(InHeight);

        m_AspectRatio = m_ViewportWidth / m_ViewportHeight;

        SetProjection();
    }

    FORCEINLINE const auto GetViewportWidth() const { return m_ViewportWidth; }
    FORCEINLINE const auto GetViewportHeight() const { return m_ViewportHeight; }

  private:
    float m_FieldOfView = 90.0f;

    float m_ViewportWidth  = 0.0f;
    float m_ViewportHeight = 0.0f;

    //  void RecalculateViewMatrix() final override {}
};
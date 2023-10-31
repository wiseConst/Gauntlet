#pragma once

#include <Gauntlet.h>

namespace Gauntlet
{

class EditorCamera final : public Gauntlet::PerspectiveCamera
{
  public:
    EditorCamera(const float InAspectRatio = 0.0f, const float InFOV = 90.0f) : Gauntlet::PerspectiveCamera(InAspectRatio, InFOV)
    {
        m_ViewportWidth  = static_cast<float>(Gauntlet::Application::Get().GetWindow()->GetWidth());
        m_ViewportHeight = static_cast<float>(Gauntlet::Application::Get().GetWindow()->GetHeight());
        SetSensitivity(0.2f);
    }

    ~EditorCamera() = default;

    FORCEINLINE static Gauntlet::Ref<EditorCamera> Create(const float InAspectRatio = 0.0f, const float InFOV = 90.0f)
    {
        return Gauntlet::Ref<EditorCamera>(new EditorCamera(InAspectRatio, InFOV));
    }

    void OnResize(float aspectRatio) final override
    {
        m_AspectRatio = aspectRatio;
        SetProjection();
    }

    void Resize(uint32_t InWidth, uint32_t InHeight)
    {
        GNT_ASSERT(InHeight != 0, "Zero division!");
        m_ViewportWidth  = static_cast<float>(InWidth);
        m_ViewportHeight = static_cast<float>(InHeight);

        OnResize(m_ViewportWidth / m_ViewportHeight);
    }

    FORCEINLINE const auto GetViewportWidth() const { return m_ViewportWidth; }
    FORCEINLINE const auto GetViewportHeight() const { return m_ViewportHeight; }

  private:
    float m_FieldOfView = 90.0f;

    float m_ViewportWidth  = 0.0f;
    float m_ViewportHeight = 0.0f;
};
}  // namespace Gauntlet
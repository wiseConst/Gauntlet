#pragma once

#include <Eclipse.h>

using namespace Eclipse;

class GravityExplorer final : public Layer
{
  public:
    GravityExplorer() : Layer("GravityExplorer") {}
    ~GravityExplorer() = default;

    void OnAttach() final override { m_Camera = Ref<OrthographicCamera>(new OrthographicCamera()); }

    void OnUpdate(const float DeltaTime) final override
    {
        m_Camera->OnUpdate(DeltaTime);
        Renderer2D::SetClearColor({0.256f, 0.2f, 0.4f, 1.0f});

        Renderer2D::BeginScene(*m_Camera);

        if (Input::IsKeyPressed(ELS_KEY_W)) m_GreenQuadPosition.y += m_Speed * DeltaTime;

        if (Input::IsKeyPressed(ELS_KEY_S)) m_GreenQuadPosition.y -= m_Speed * DeltaTime;

        if (Input::IsKeyPressed(ELS_KEY_D)) m_GreenQuadPosition.x += m_Speed * DeltaTime;

        if (Input::IsKeyPressed(ELS_KEY_A)) m_GreenQuadPosition.x -= m_Speed * DeltaTime;

        Renderer2D::DrawQuad(m_GreenQuadPosition, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f});
        Renderer2D::DrawQuad({1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f});
    }

    void OnEvent(Event& InEvent) final override { m_Camera->OnEvent(InEvent); }

    void OnDetach() final override {}

  private:
    Ref<OrthographicCamera> m_Camera;

    const float m_Speed = 2.0f;
    glm::vec3 m_GreenQuadPosition = {0.0f, 0.0f, 0.0f};
};

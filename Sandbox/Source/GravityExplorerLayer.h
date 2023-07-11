#pragma once

#include <Eclipse.h>

using namespace Eclipse;

class GravityExplorer final : public Layer
{
  public:
    GravityExplorer() : Layer("GravityExplorer") {}
    ~GravityExplorer() = default;

    void OnAttach() final override
    {
        m_Camera = Ref<OrthographicCamera>(new OrthographicCamera());
        m_ShipTexture = Texture2D::Create("Resources/Textures/Ship.png");
        m_BoardTexture = Texture2D::Create("Resources/Textures/Checkerboard.png");
    }

    void OnUpdate(const float DeltaTime) final override
    {
        m_Camera->OnUpdate(DeltaTime);
        Renderer2D::SetClearColor({0.5f, 0.3f, 0.67f, 1.0f});

        Renderer2D::BeginScene(*m_Camera);

        if (Input::IsKeyPressed(ELS_KEY_W)) m_ShipPosition.y += m_ShipSpeed * DeltaTime;

        if (Input::IsKeyPressed(ELS_KEY_S)) m_ShipPosition.y -= m_ShipSpeed * DeltaTime;

        if (Input::IsKeyPressed(ELS_KEY_D)) m_ShipPosition.x += m_ShipSpeed * DeltaTime;

        if (Input::IsKeyPressed(ELS_KEY_A)) m_ShipPosition.x -= m_ShipSpeed * DeltaTime;

        UpdateCollision(DeltaTime);

        Renderer2D::DrawTexturedQuad(m_ShipPosition, m_ShipSize, m_ShipTexture);
        Renderer2D::DrawTexturedQuad({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, m_BoardTexture);
        Renderer2D::DrawTexturedQuad({-3.0f, -3.0f, 0.0f}, {1.2f, 1.2f}, m_BoardTexture, {1.0f, 0.5f, 0.5f, 0.5f});
        Renderer2D::DrawTexturedQuad({-8.0f, -8.0f, 0.0f}, {1.2f, 1.2f}, m_BoardTexture, {0.0f, 0.5f, 1.0f, 0.5f});
        Renderer2D::DrawTexturedQuad({8.0f, 8.0f, 0.0f}, {1.2f, 1.2f}, m_BoardTexture, {1.0f, 0.5f, 0.2f, 0.5f});
    }

    void OnEvent(Event& InEvent) final override { m_Camera->OnEvent(InEvent); }

    void OnImGuiRender() final override
    {
        static bool bUpdateGravityExplorer = true;

        if (bUpdateGravityExplorer)
        {
            ImGui::Begin("GravityExplorer", &bUpdateGravityExplorer);
            ImGui::SliderFloat("Ship Speed", &m_ShipSpeed, 1.0f, 10.0f);
            ImGui::End();
        }
    }

    void OnDetach() final override
    {
        m_ShipTexture->Destroy();
        m_BoardTexture->Destroy();
    }

  private:
    Ref<OrthographicCamera> m_Camera;

    Ref<Texture2D> m_BoardTexture;

    Ref<Texture2D> m_ShipTexture;
    glm::vec3 m_ShipPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec2 m_ShipSize = glm::vec2(1.0f, 1.0f);
    float m_ShipSpeed = 4.2f;

    void UpdateCollision(const float DeltaTime)
    {
        // Window bounding box
        if (m_ShipPosition.x + m_ShipSize.x / 2 > m_Camera->GetZoomLevel() * m_Camera->GetAspectRatio())
            m_ShipPosition.x = m_Camera->GetZoomLevel() * m_Camera->GetAspectRatio() - m_ShipSize.x / 2;

        if (m_ShipPosition.x - m_ShipSize.x / 2 < -m_Camera->GetZoomLevel() * m_Camera->GetAspectRatio())
            m_ShipPosition.x = -m_Camera->GetZoomLevel() * m_Camera->GetAspectRatio() + m_ShipSize.x / 2;

        if (m_ShipPosition.y + m_ShipSize.y / 2 > m_Camera->GetZoomLevel()) m_ShipPosition.y = m_Camera->GetZoomLevel() - m_ShipSize.y / 2;
        if (m_ShipPosition.y - m_ShipSize.y / 2 < -m_Camera->GetZoomLevel())
            m_ShipPosition.y = -m_Camera->GetZoomLevel() + m_ShipSize.y / 2;
    }
};

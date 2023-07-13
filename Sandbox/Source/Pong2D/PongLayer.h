#pragma once

#include <Eclipse.h>

using namespace Eclipse;

enum class EGameState : uint8_t
{
    GAME_STATE_MENU = 0,
    GAME_STATE_IN_PROGRESS,
    GAME_STATE_GAME_OVER
};

class PongLayer final : public Layer
{
  public:
    PongLayer() : Layer("PongLayer") {}
    ~PongLayer() = default;

    void OnAttach() final override
    {
        m_Camera = OrthographicCamera::Create();

        m_LeftPaddlePosition.x  = -m_Camera->GetWidthBorder() + m_PaddleSize.x;
        m_RightPaddlePosition.x = m_Camera->GetWidthBorder() - m_PaddleSize.x;

        m_BackgroundTexture = Texture2D::Create("Resources/Textures/Cities/2.png");
        m_BallTexture       = Texture2D::Create("Resources/Textures/Snowball.png");
    }

    void OnUpdate(const float DeltaTime) final override
    {
        m_Camera->OnUpdate(DeltaTime);
        Renderer2D::SetClearColor({0.1f, 0.1f, 0.1f, 1.0f});

        Renderer2D::BeginScene(*m_Camera);

        m_Time += DeltaTime;
        if ((int)(m_Time * 10.0f) % 8 > 4) m_Blink = !m_Blink;

        switch (m_GameState)
        {
            case EGameState::GAME_STATE_IN_PROGRESS:
            {
                UpdateInput(DeltaTime);
                UpdateCollision(DeltaTime);

                m_RightPaddlePosition.y = std::clamp(m_BallPosition.y, -m_Camera->GetHeightBorder() + m_PaddleSize.y / 2,
                                                     m_Camera->GetHeightBorder() - m_PaddleSize.y / 2);

                m_BallPosition += m_BallVelocity * DeltaTime;
                break;
            }
        }

        Renderer2D::DrawTexturedQuad(glm::vec3(0.0f),
                                     {m_BackgroundTexture->GetWidth() / m_Camera->GetAspectRatio() / m_Camera->GetZoomLevel(),
                                      m_BackgroundTexture->GetHeight() / m_Camera->GetZoomLevel()},
                                     m_BackgroundTexture);

        Renderer2D::DrawQuad(m_LeftPaddlePosition, m_PaddleSize, {1.0f, 1.0f, 1.0f, 1.0f});

        Renderer2D::DrawQuad(m_RightPaddlePosition, m_PaddleSize, {1.0f, 1.0f, 1.0f, 1.0f});

        Renderer2D::DrawTexturedQuad(m_BallPosition, m_BallSize, m_BallTexture, glm::vec4(0.3f, 0.9f, 0.22f, 1.0f));
    }

    void OnEvent(Event& InEvent) final override
    {
        m_Camera->OnEvent(InEvent);

        if (InEvent.GetType() == EEventType::MouseButtonPressedEvent && m_GameState == EGameState::GAME_STATE_GAME_OVER)
        {
            m_GameState = EGameState::GAME_STATE_IN_PROGRESS;
            m_Score     = 0;

            m_LeftPaddlePosition.x  = -m_Camera->GetWidthBorder() + m_PaddleSize.x;
            m_RightPaddlePosition.x = m_Camera->GetWidthBorder() - m_PaddleSize.x;

            m_BallPosition = glm::vec3(0.0f);
            m_BallVelocity = glm::vec2(11.0f);
        }

        // Correct paddle position if window resized
        if (InEvent.GetType() == EEventType::WindowResizeEvent)
        {
            m_LeftPaddlePosition.x  = -m_Camera->GetWidthBorder() + m_PaddleSize.x;
            m_RightPaddlePosition.x = m_Camera->GetWidthBorder() - m_PaddleSize.x;
        }
    }

    void OnImGuiRender() final override
    {

        switch (m_GameState)
        {
            case EGameState::GAME_STATE_IN_PROGRESS:
            {
                const std::string ScoreText = std::string("Score: ") + std::to_string(m_Score);
                ImGui::GetForegroundDrawList()->AddText(/*m_Font, 48.0f,*/ ImGui::GetWindowPos(), 0xffffffff, ScoreText.data());
                break;
            }
            case EGameState::GAME_STATE_MENU:
            {
                if (Input::IsKeyPressed(ELS_KEY_SPACE) || Input::IsMouseButtonPressed(ELS_MOUSE_BUTTON_1))
                {
                    m_GameState = EGameState::GAME_STATE_IN_PROGRESS;
                }

                auto pos    = ImGui::GetWindowPos();
                auto width  = Application::Get().GetWindow()->GetWidth();
                auto height = Application::Get().GetWindow()->GetHeight();
                pos.x += width * 0.5f - 300.0f;
                pos.y += 50.0f;
                if (m_Blink) ImGui::GetForegroundDrawList()->AddText(/*m_Font, 120.0f,*/ pos, 0xffffffff, "Click to Play!");
                break;
            }
            case EGameState::GAME_STATE_GAME_OVER:
            {
                auto pos    = ImGui::GetWindowPos();
                auto width  = Application::Get().GetWindow()->GetWidth();
                auto height = Application::Get().GetWindow()->GetHeight();
                pos.x += width * 0.5f - 300.0f;
                pos.y += 50.0f;
                if (m_Blink) ImGui::GetForegroundDrawList()->AddText(/*m_Font, 120.0f*/ pos, 0xffffffff, "Click <MB1> to Play!");

                pos.x += 200.0f;
                pos.y += 150.0f;
                const std::string ScoreText = std::string("Score: ") + std::to_string(m_Score);
                ImGui::GetForegroundDrawList()->AddText(/*m_Font, 48.0f,*/ pos, 0xffffffff, ScoreText.data());
                break;
            }
        }

#if ELS_DEBUG
        static bool bShowPongStats = true;
        if (bShowPongStats)
        {
            ImGui::Begin("2D Pong-Game Stats", &bShowPongStats);
            ImGui::Text("Ball Position: (%f,%f)", m_BallPosition.x, m_BallPosition.y);
            ImGui::Text("Score: %u", m_Score);
            ImGui::End();
        }

        static bool bShowAppStats = false;
        if (bShowAppStats)
        {
            ImGui::Begin("Application Stats", &bShowAppStats);

            const auto& Stats = Renderer2D::GetStats();
            ImGui::Text("Allocated Images: %llu", Stats.AllocatedImages);
            ImGui::Text("Allocated Buffers: %llu", Stats.AllocatedBuffers);
            ImGui::Text("VRAM Usage: %f MB", Stats.GPUMemoryAllocated / 1024.0f / 1024.0f);
            ImGui::Text("CPU Wait Time: %f(ms)", Stats.CPUWaitTime);
            ImGui::Text("GPU Wait Time: %f(ms)", Stats.GPUWaitTime);
            ImGui::Text("VMA Allocations: %llu", Stats.Allocations);
            ImGui::Text("DrawCalls: %llu", Stats.DrawCalls);
            ImGui::Text("QuadCount: %llu", Stats.QuadCount);

            ImGui::End();
        }
#endif
    }

    void OnDetach() final override
    {
        m_BackgroundTexture->Destroy();
        m_BallTexture->Destroy();
    }

  private:
    Ref<OrthographicCamera> m_Camera;
    Ref<Texture2D> m_BackgroundTexture;

    glm::vec2 m_LeftPaddlePosition  = glm::vec2(1.0f, 1.0f);
    glm::vec2 m_RightPaddlePosition = glm::vec2(1.0f, 1.0f);

    glm::vec2 m_PaddleSize = glm::vec2(1.0f, 3.0f);
    float m_PaddleSpeed    = 12.0f;

    Ref<Texture2D> m_BallTexture;
    glm::vec2 m_BallPosition = glm::vec2(0.0f, 0.0f);  // Middle of the screen
    glm::vec2 m_BallSize     = glm::vec2(1.0f);
    glm::vec2 m_BallVelocity = glm::vec2(11.0f);

    uint32_t m_Score       = 0;
    EGameState m_GameState = EGameState::GAME_STATE_MENU;
    bool m_Blink{false};
    float m_Time = 0.0f;

    void UpdateCollision(const float DeltaTime)
    {
        // Left paddle(player) collides with window bottom && top borders
        {
            if (m_LeftPaddlePosition.y + m_PaddleSize.y / 2 >= m_Camera->GetHeightBorder())
                m_LeftPaddlePosition.y = m_Camera->GetHeightBorder() - m_PaddleSize.y / 2;

            if (m_LeftPaddlePosition.y - m_PaddleSize.y / 2 <= -m_Camera->GetHeightBorder())
                m_LeftPaddlePosition.y = -m_Camera->GetHeightBorder() + m_PaddleSize.y / 2;
        }

        // Right paddle(AI) collides with window bottom && top borders
        {
            if (m_RightPaddlePosition.y + m_PaddleSize.y / 2 >= m_Camera->GetHeightBorder())
                m_RightPaddlePosition.y = m_Camera->GetHeightBorder() - m_PaddleSize.y / 2;

            if (m_RightPaddlePosition.y - m_PaddleSize.y / 2 <= -m_Camera->GetHeightBorder())
                m_RightPaddlePosition.y = -m_Camera->GetHeightBorder() + m_PaddleSize.y / 2;
        }

        // Ball collides with window borders
        {
            if (m_BallPosition.y + m_BallSize.y / 2 >= m_Camera->GetHeightBorder())
            {
                m_BallPosition.y = m_Camera->GetHeightBorder() - m_BallSize.y / 2;
                m_BallVelocity.y *= -1;
            }

            if (m_BallPosition.y - m_BallSize.y / 2 <= -m_Camera->GetHeightBorder())
            {
                m_BallPosition.y = -m_Camera->GetHeightBorder() + m_BallSize.y / 2;
                m_BallVelocity.y *= -1;
            }

            if (m_BallPosition.x + m_BallSize.x / 2 >= m_Camera->GetWidthBorder())
            {
                m_BallPosition.x = m_Camera->GetWidthBorder() - m_BallSize.x / 2;
                m_BallVelocity.x *= -1;
                m_GameState = EGameState::GAME_STATE_GAME_OVER;
            }

            if (m_BallPosition.x - m_BallSize.x / 2 <= -m_Camera->GetWidthBorder())
            {
                m_BallPosition.x = -m_Camera->GetWidthBorder() + m_BallSize.x / 2;
                m_BallVelocity.x *= -1;
                m_GameState = EGameState::GAME_STATE_GAME_OVER;
            }
        }

        // Ball collides with paddles
        {
            const float BallRightBorder  = m_BallPosition.x + m_BallSize.x / 2;
            const float BallLeftBorder   = m_BallPosition.x - m_BallSize.x / 2;
            const float BallBottomBorder = m_BallPosition.y - m_BallSize.y / 2;
            const float BallTopBorder    = m_BallPosition.y + m_BallSize.y / 2;

            // Player
            {
                const float PaddleRightBorder  = m_LeftPaddlePosition.x + m_PaddleSize.x / 2;
                const float PaddleLeftBorder   = m_LeftPaddlePosition.x - m_PaddleSize.x / 2;
                const float PaddleBottomBorder = m_LeftPaddlePosition.y - m_PaddleSize.y / 2;
                const float PaddleTopBorder    = m_LeftPaddlePosition.y + m_PaddleSize.y / 2;

                const bool bIsPlayerCollides = PaddleRightBorder >= BallLeftBorder &&  //
                                               PaddleLeftBorder <= BallRightBorder &&  //
                                               PaddleTopBorder >= BallBottomBorder &&  //
                                               PaddleBottomBorder <= BallTopBorder;

                if (bIsPlayerCollides)
                {
                    m_BallVelocity.x *= -1;
                    ++m_Score;
                    m_BallVelocity += m_Score * 0.1f;

                    m_BallPosition.x = PaddleRightBorder + m_BallSize.x / 2;  // Corner case issue solution
                }
            }

            // AI
            {
                const float PaddleRightBorder  = m_RightPaddlePosition.x + m_PaddleSize.x / 2;
                const float PaddleLeftBorder   = m_RightPaddlePosition.x - m_PaddleSize.x / 2;
                const float PaddleBottomBorder = m_RightPaddlePosition.y - m_PaddleSize.y / 2;
                const float PaddleTopBorder    = m_RightPaddlePosition.y + m_PaddleSize.y / 2;

                const bool bIsPlayerCollides = PaddleRightBorder >= BallLeftBorder &&  //
                                               PaddleLeftBorder <= BallRightBorder &&  //
                                               PaddleTopBorder >= BallBottomBorder &&  //
                                               PaddleBottomBorder <= BallTopBorder;

                if (bIsPlayerCollides)
                {
                    m_BallVelocity.x *= -1;

                    m_BallPosition.x = PaddleLeftBorder - m_BallSize.x / 2;  // Corner case issue solution
                }
            }
        }
    }

    void UpdateInput(const float DeltaTime)
    {
        if (Input::IsKeyPressed(ELS_KEY_W)) m_LeftPaddlePosition.y += m_PaddleSpeed * DeltaTime;

        if (Input::IsKeyPressed(ELS_KEY_S)) m_LeftPaddlePosition.y -= m_PaddleSpeed * DeltaTime;
    }
};

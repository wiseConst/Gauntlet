#pragma once

#include "Eclipse.h"

#include "Level.h"
#include <imgui/imgui.h>

class GameLayer final : public Eclipse::Layer
{
  public:
    GameLayer();
    ~GameLayer() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;

    void OnUpdate(const float DeltaTime) override;
    virtual void OnImGuiRender() override;
    void OnEvent(Eclipse::Event& e) override;
    bool OnMouseButtonPressed(Eclipse::MouseButtonPressedEvent& e);
    bool OnWindowResize(Eclipse::WindowResizeEvent& e);

  private:
    void CreateCamera(uint32_t width, uint32_t height);

  private:
    Eclipse::Ref<Eclipse::OrthographicCamera> m_Camera;
    Level m_Level;
    ImFont* m_Font = nullptr;
    float m_Time   = 0.0f;
    bool m_Blink   = false;

    enum class GameState
    {
        Play     = 0,
        MainMenu = 1,
        GameOver = 2
    };

    GameState m_State = GameState::MainMenu;
};
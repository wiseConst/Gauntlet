#pragma once

#include <Gauntlet.h>
#include "EditorCamera.h"

using namespace Gauntlet;

class RuntimeLayer final : public Layer
{
  public:
    RuntimeLayer();
    ~RuntimeLayer() = default;

    void OnAttach() final override;

    void OnDetach() final override;

    void OnUpdate(const float deltaTime) final override;

    void OnEvent(Event& event) final override;

    void OnImGuiRender() final override;

  private:
    Ref<EditorCamera> m_EditorCamera;
    Scoped<ImGuiLayer>& m_GUILayer;

    Ref<Scene> m_ActiveScene;

    ImVec2 m_ViewportSize     = ImVec2(0.0f, 0.0f);
    bool m_bIsViewportFocused = false;
    bool m_bIsViewportHovered = false;

    void BeginDockspace();

    void EndDockspace();

    void UpdateViewportSize();
};
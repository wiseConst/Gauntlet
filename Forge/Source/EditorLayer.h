#pragma once

#include <Gauntlet.h>
#include "EditorCamera.h"
#include "Panels/SceneHierarchyPanel.h"

using namespace Gauntlet;

class EditorLayer final : public Layer
{
  public:
    EditorLayer();
    ~EditorLayer() = default;

    void OnAttach() final override;

    void OnDetach() final override;

    void OnUpdate(const float DeltaTime) final override;

    void OnEvent(Event& InEvent) final override;

    void OnImGuiRender() final override;

  private:
    Ref<EditorCamera> m_EditorCamera;
    const Scoped<ImGuiLayer>& m_GUILayer;

    Ref<Scene> m_ActiveScene;

    ImVec2 m_ViewportSize     = ImVec2(0.0f, 0.0f);
    bool m_bIsViewportFocused = false;
    bool m_bIsViewportHovered = false;

    // Panels
    SceneHierarchyPanel m_SceneHierarchyPanel;

    void BeginDockspace();

    void EndDockspace();

    void UpdateViewportSize();
};
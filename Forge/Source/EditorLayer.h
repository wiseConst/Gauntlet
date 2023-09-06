#pragma once

#include <Gauntlet.h>
#include "EditorCamera.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"

namespace Gauntlet
{

class EditorLayer final : public Layer
{
  public:
    EditorLayer();
    ~EditorLayer() = default;

    void OnAttach() final override;

    void OnDetach() final override;

    void OnUpdate(const float deltaTime) final override;

    void OnEvent(Event& event) final override;

    void OnImGuiRender() final override;

  private:
    Ref<EditorCamera> m_EditorCamera;
    Scoped<ImGuiLayer>& m_GUILayer;

    Ref<Scene> m_ActiveScene;

    enum class ESceneState : uint8_t
    {
        Edit = 0,
        Play = 1
    } m_SceneState = ESceneState::Edit;

    ImVec2 m_ViewportSize     = ImVec2(0.0f, 0.0f);
    bool m_bIsViewportFocused = false;
    bool m_bIsViewportHovered = false;

    Ref<Texture2D> m_PlayIcon{nullptr};
    Ref<Texture2D> m_StopIcon{nullptr};

    // Panels
    SceneHierarchyPanel m_SceneHierarchyPanel;
    ContentBrowserPanel m_ContentBrowserPanel;

    // Dockspacing
    void BeginDockspace();
    void EndDockspace();

    void UpdateViewportSize();
    void UI_Toolbar();

    // Scenes
    void NewScene();
    void SaveScene();

    void OpenScene();
    void OpenScene(const std::filesystem::path& path);

    void OnSceneStop() { m_SceneState = ESceneState::Edit; }
    void OnScenePlay() { m_SceneState = ESceneState::Play; }

    // Events
    void OnKeyPressed(KeyButtonPressedEvent& e);
};
}  // namespace Gauntlet
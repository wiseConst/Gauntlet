#include "EditorLayer.h"
#include "Gauntlet/Scene/SceneSerializer.h"
#include "Gauntlet/Utils/PlatformUtils.h"

namespace Gauntlet
{
#define SPONZA_TEST 1
#define TEST_BED 0

extern const std::filesystem::path g_AssetsPath = "Resources";

EditorLayer::EditorLayer() : Layer("EditorLayer"), m_GUILayer(Application::Get().GetGUILayer()) {}

void EditorLayer::OnAttach()
{
    m_EditorCamera = EditorCamera::Create();

#if SPONZA_TEST
#if 0
    m_ActiveScene = MakeRef<Scene>("SponzaTest");

    {
        auto Sponza = m_ActiveScene->CreateEntity("Sponza");
        Sponza.AddComponent<MeshComponent>(Mesh::Create("Resources/Models/Sponza/sponza.gltf"));
    }
#endif

    SceneSerializer serializer(m_ActiveScene);
    GNT_ASSERT(serializer.Deserialize("Resources/Scenes/SponzaTest.gntlt"), "Failed to deserialize scene!");

#endif

#if TEST_BED
    SceneSerializer serializer(m_ActiveScene);
    GNT_ASSERT(serializer.Deserialize("Resources/Scenes/TestBed.gntlt"), "Failed to deserialize scene!");
#endif

    m_SceneHierarchyPanel.SetContext(m_ActiveScene);

    m_PlayIcon = Texture2D::Create("Resources/Icons/icons8/play-48.png", true);
    m_StopIcon = Texture2D::Create("Resources/Icons/icons8/stop-48.png", true);
}

void EditorLayer::OnDetach()
{
    SceneSerializer serializer(m_ActiveScene);
    serializer.Serialize("Resources/Scenes/" + m_ActiveScene->GetName() + ".gntlt");

    m_PlayIcon->Destroy();
    m_StopIcon->Destroy();
}

void EditorLayer::OnUpdate(const float deltaTime)
{
    if (m_bIsViewportFocused) m_EditorCamera->OnUpdate(deltaTime);

    Renderer::BeginScene(*m_EditorCamera);

    m_ActiveScene->OnUpdate(deltaTime);

    Renderer::EndScene();
}

void EditorLayer::OnEvent(Event& event)
{
    if (m_bIsViewportHovered) m_EditorCamera->OnEvent(event);

    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<KeyButtonPressedEvent>(BIND_FN(EditorLayer::OnKeyPressed));
}

void EditorLayer::OnKeyPressed(KeyButtonPressedEvent& e)
{
    if (e.IsRepeat()) return;

    const bool bShiftPressed = Input::IsKeyPressed(KeyCode::KEY_LEFT_SHIFT) || Input::IsKeyPressed(KeyCode::KEY_RIGHT_SHIFT);
    const bool bCtrlPressed  = Input::IsKeyPressed(KeyCode::KEY_RIGHT_CONTROL) || Input::IsKeyPressed(KeyCode::KEY_LEFT_CONTROL);

    if (bShiftPressed && bCtrlPressed && Input::IsKeyPressed(KeyCode::KEY_N)) NewScene();

    if (bCtrlPressed && Input::IsKeyPressed(KeyCode::KEY_O)) OpenScene();

    if (bShiftPressed && bCtrlPressed && Input::IsKeyPressed(KeyCode::KEY_S)) SaveScene();
}

void EditorLayer::OnImGuiRender()
{
    UpdateViewportSize();

    BeginDockspace();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
    ImGui::Begin("Viewport");
    ImGui::PopStyleVar();
    m_ViewportSize = ImGui::GetContentRegionAvail();

    m_bIsViewportFocused = ImGui::IsWindowFocused();
    m_bIsViewportHovered = ImGui::IsWindowHovered();
    m_GUILayer->BlockEvents(m_bIsViewportFocused && !m_bIsViewportHovered);

    ImGui::Image(Renderer::GetFinalImage()->GetTextureID(), m_ViewportSize);

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
        {
            const wchar_t* path = (const wchar_t*)payload->Data;
            const std::filesystem::path scenePath(path);

            if (scenePath.extension().string() == std::string(".gntlt")) OpenScene(scenePath.string());
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::End();

    // static bool ShowDemoWindow = true;
    // if (ShowDemoWindow) ImGui::ShowDemoWindow(&ShowDemoWindow);

    static bool bShowAppStats = true;
    if (bShowAppStats)
    {
        ImGui::Begin("Application Stats", &bShowAppStats);

        const auto& Stats = Renderer::GetStats();
        ImGui::Text("Allocated Images: %llu", Stats.AllocatedImages.load());
        ImGui::Text("Allocated Buffers: %llu", Stats.AllocatedBuffers.load());
        ImGui::Text("Allocated StagingVertexBuffers: %llu", Stats.StagingVertexBuffers.load());
        ImGui::Text("VRAM Usage: (%0.2f) MB", Stats.GPUMemoryAllocated.load() / 1024.0f / 1024.0f);
        ImGui::Text("FPS: (%u)", Stats.FPS);
        ImGui::Text("Allocated Descriptor Sets: (%u)", Stats.AllocatedDescriptorSets.load());
        ImGui::Text("CPU Wait Time: %0.2f ms", Stats.CPUWaitTime * 1000.0f);
        ImGui::Text("GPU Wait Time: %0.2f ms", Stats.GPUWaitTime * 1000.0f);
        ImGui::Text("FrameTime: %0.2f ms", Stats.FrameTime * 1000.0f);
        ImGui::Text("VMA Allocations: %llu", Stats.Allocations.load());
        ImGui::Text("DrawCalls: %llu", Stats.DrawCalls.load());
        ImGui::Text("QuadCount: %llu", Stats.QuadCount.load());

        ImGui::End();
    }

    ImGui::Begin("Renderer Settings");
    auto& rs = Renderer::GetSettings();
    ImGui::Checkbox("Render Wireframe", &rs.ShowWireframes);
    ImGui::Checkbox("VSync", &rs.VSync);
    ImGui::Checkbox("Render Shadows", &rs.RenderShadows);
    ImGui::Checkbox("Display ShadowMap", &rs.DisplayShadowMapRenderTarget);
    ImGui::SliderFloat("Gamma", &rs.Gamma, 1.0f, 2.6f, "%0.1f");
    ImGui::End();

    ImGui::Begin("Renderer Outputs");
    for (auto& rendererOuput : Renderer::GetRendererOutput())
    {
        ImGui::Text(rendererOuput.Name.data());
        ImGui::Image(rendererOuput.Attachment->GetTextureID(),
                     {ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().x / 2});
        ImGui::Separator();
    }

    ImGui::End();

    m_SceneHierarchyPanel.OnImGuiRender();

    m_ContentBrowserPanel.OnImGuiRender();

    UI_Toolbar();

    EndDockspace();
}

void EditorLayer::UI_Toolbar()
{
    // TODO: Currently unused
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

    ImGui::Begin("##toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    const float size = ImGui::GetWindowHeight() - 4.0f;
    const auto icon  = m_SceneState == ESceneState::Edit ? m_PlayIcon : m_StopIcon;
    ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x * 0.5f - size * 0.5f);
    if (ImGui::ImageButton(icon->GetTextureID(), ImVec2(size, size)))
    {
        if (m_SceneState == ESceneState::Edit)
            OnScenePlay();
        else if (m_SceneState == ESceneState::Play)
            OnSceneStop();
    }
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void EditorLayer::BeginDockspace()
{
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
    static bool bShowDockSpace                = true;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
    // and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", &bShowDockSpace, window_flags);
    ImGui::PopStyleVar(3);

    // Submit the DockSpace
    ImGuiIO& io                   = ImGui::GetIO();
    ImGuiStyle& style             = ImGui::GetStyle();
    const float PrevWindowMinSize = style.WindowMinSize.x;
    style.WindowMinSize.x         = 360.0f;
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }
    style.WindowMinSize.x = PrevWindowMinSize;

    const float SceneNameRegionX = ImGui::GetContentRegionAvail().x - m_ActiveScene->GetName().length() * ImGui::GetFont()->FontSize * 4.0f;
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Editor Settings"))
        {
            if (ImGui::MenuItem("New", "Ctrl+Shift+N")) NewScene();

            if (ImGui::MenuItem("Open ...", "Ctrl+O")) OpenScene();

            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) SaveScene();

            if (ImGui::MenuItem("Exit Editor")) Application::Get().Close();
            ImGui::EndMenu();
        }

        // Scene name
        ImGui::SameLine(SceneNameRegionX);
        ImGui::PushStyleColor(ImGuiCol_Button, {0.2f, 0.205f, 0.21f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.2f, 0.205f, 0.21f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.2f, 0.205f, 0.21f, 1.0f});
        ImGui::Button(m_ActiveScene->GetName().data());
        ImGui::PopStyleColor(3);

        ImGui::EndMenuBar();
    }
}

void EditorLayer::EndDockspace()
{
    ImGui::End();
}

void EditorLayer::UpdateViewportSize()
{
    const auto bNeedViewportResize =
        m_EditorCamera->GetViewportWidth() != m_ViewportSize.x || m_EditorCamera->GetViewportHeight() != m_ViewportSize.y;
    const auto bIsViewportValid = m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f;

    if (bNeedViewportResize && bIsViewportValid)
    {
        Renderer::ResizeFramebuffers((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
        m_EditorCamera->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
    }
}

void EditorLayer::NewScene()
{
    m_ActiveScene = MakeRef<Scene>();
    m_SceneHierarchyPanel.SetContext(m_ActiveScene);
}

void EditorLayer::OpenScene()
{
    const std::string filePath = FileDialogs::OpenFile("Gauntlet Scene (*.gntlt)\0*.gntlt\0");

    if (filePath.empty())
        LOG_WARN("Failed to open scene: %s", filePath.data());
    else
        OpenScene(filePath);
}

void EditorLayer::OpenScene(const std::filesystem::path& path)
{
    m_ActiveScene = MakeRef<Scene>();

    SceneSerializer serializer(m_ActiveScene);
    serializer.Deserialize(path.string());

    m_SceneHierarchyPanel.SetContext(m_ActiveScene);
}

void EditorLayer::SaveScene()
{
    const std::string filePath = FileDialogs::SaveFile("Gauntlet Scene (*.gntlt)\0*.gntlt\0");

    if (filePath.empty())
        LOG_WARN("Failed to save scene: %s", filePath.data());
    else
    {
        SceneSerializer serializer(m_ActiveScene);
        serializer.Serialize(filePath);
    }
}
}  // namespace Gauntlet
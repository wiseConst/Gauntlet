#include "EditorLayer.h"
#include "Gauntlet/Scene/SceneSerializer.h"
#include "Gauntlet/Utils/PlatformUtils.h"

namespace Gauntlet
{
extern const std::filesystem::path g_AssetsPath = "Resources";

EditorLayer::EditorLayer() : Layer("EditorLayer"), m_GUILayer(Application::Get().GetGUILayer()) {}

void EditorLayer::OnAttach()
{
    m_EditorCamera = EditorCamera::Create();
    m_ActiveScene  = MakeRef<Scene>("EmptyScene");

    m_SceneHierarchyPanel.SetContext(m_ActiveScene);
}

void EditorLayer::OnDetach()
{
    SceneSerializer serializer(m_ActiveScene);
    serializer.Serialize("Resources/Scenes/" + m_ActiveScene->GetName() + ".gntlt");
}

void EditorLayer::OnUpdate(const float deltaTime)
{
    if (m_bIsViewportFocused) m_EditorCamera->OnUpdate(deltaTime);

    UpdateViewportSize();

    Renderer::BeginScene(*m_EditorCamera);

    m_ActiveScene->OnUpdate(deltaTime);

    Renderer::EndScene();
}

void EditorLayer::OnEvent(Event& event)
{
    if (m_bIsViewportHovered || m_bIsViewportFocused) m_EditorCamera->OnEvent(event);

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

    if (Input::IsKeyPressed(KeyCode::KEY_P)) Renderer::GetSettings().ShowWireframes = !Renderer::GetSettings().ShowWireframes;
}

void EditorLayer::OnImGuiRender()
{
    BeginDockspace();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
    ImGui::Begin("Viewport");

    m_ViewportSize = ImGui::GetContentRegionAvail();

    m_bIsViewportFocused = ImGui::IsWindowFocused();
    m_bIsViewportHovered = ImGui::IsWindowHovered();
    //     LOG_WARN("Hover: %d / Focus: %d", m_bIsViewportHovered, m_bIsViewportFocused);

    m_GUILayer->BlockEvents(!m_bIsViewportHovered);

    ImGui::Image(Renderer::GetFinalImage()->GetTextureID(), m_ViewportSize);

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
        {
            const wchar_t* path = (const wchar_t*)payload->Data;
            const std::filesystem::path fsPath(path);

            if (fsPath.extension().string() == std::string(".gntlt"))
                OpenScene(fsPath.string());
            else if (fsPath.extension().string() == std::string(".gltf"))
            {
                std::string meshPath = fsPath.string();
                std::replace(meshPath.begin(), meshPath.end(), '\\', '/');
                m_ActiveScene->CreateEntity("New Entity").AddComponent<MeshComponent>(meshPath);
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::PopStyleVar();
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
        ImGui::Text("VRAM Usage: (%0.2f) MB", Stats.GPUMemoryAllocated.load() / 1024.0f / 1024.0f);
        ImGui::Text("RAM Usage: (%0.2f) MB", Stats.RAMMemoryAllocated.load() / 1024.0f / 1024.0f);
        ImGui::Text("FPS: (%u)", Stats.FPS);
        ImGui::Text("Allocated Descriptor Sets: (%u)", Stats.AllocatedDescriptorSets.load());
        ImGui::Text("CPU Wait Time: %0.2f ms", Stats.CPUWaitTime * 1000.0f);
        ImGui::Text("GPU Wait Time: %0.2f ms", Stats.GPUWaitTime * 1000.0f);
        ImGui::Text("Swapchain Image Present Time: %0.2fms", Stats.PresentTime * 1000.0f);
        ImGui::Text("FrameTime: %0.2f ms", Stats.FrameTime * 1000.0f);
        ImGui::Text("VMA Allocations: %llu", Stats.Allocations.load());
        ImGui::Text("DrawCalls: %llu", Stats.DrawCalls.load());
        ImGui::Text("QuadCount: %llu", Stats.QuadCount.load());
        ImGui::Text("Rendering Device: %s", Stats.RenderingDevice.data());
        ImGui::Text("Upload Heap Capacity: (%0.2f) MB", Stats.UploadHeapCapacity / 1024.0f / 1024.0f);

        ImGui::End();
    }

    ImGui::Begin("Renderer Settings");
    auto& rs = Renderer::GetSettings();
    ImGui::Checkbox("Render Wireframe", &rs.ShowWireframes);
    ImGui::Checkbox("ChromaticAberration View", &rs.ChromaticAberrationView);
    ImGui::Checkbox("VSync", &rs.VSync);
    ImGui::SliderFloat("Gamma", &rs.Gamma, 1.0f, 2.6f, "%0.1f");
    //   ImGui::SliderFloat("Exposure", &rs.Exposure, 0.0f, 5.0f, "%0.1f");
    if (ImGui::TreeNodeEx("Shadows", ImGuiTreeNodeFlags_Framed))
    {
        ImGui::Checkbox("Render Shadows", &rs.Shadows.RenderShadows);
        ImGui::Checkbox("Soft Shadows", &rs.Shadows.SoftShadows);

        ImGui::Text("Shadow Quality:");
        ImGui::SameLine();
        if (ImGui::BeginCombo("##ShadowQuality", rs.Shadows.ShadowPresets[rs.Shadows.CurrentShadowPreset].first.data()))
        {
            for (uint16_t i = 0; i < rs.Shadows.ShadowPresets.size(); ++i)
            {
                if (ImGui::Selectable(rs.Shadows.ShadowPresets[i].first.data())) rs.Shadows.CurrentShadowPreset = i;
            }
            ImGui::EndCombo();
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("SSAO", ImGuiTreeNodeFlags_Framed))
    {
        ImGui::Checkbox("Enable SSAO", &rs.AO.EnableSSAO);
        ImGui::Checkbox("Enable SSAO-Blur", &rs.AO.BlurSSAO);
        ImGui::SliderFloat("Sample Radius", &rs.AO.Radius, 0.5f, 5.0f);
        ImGui::SliderFloat("Sample Bias", &rs.AO.Bias, 0.0f, 2.0f);
        ImGui::SliderInt("Magnitude", &rs.AO.Magnitude, 0, 15);

        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Pipeline Statistics", ImGuiTreeNodeFlags_Framed))
    {
        const auto passStats = Renderer::GetPassStatistics();
        for (auto& passStat : passStats)
            ImGui::Text("%s", passStat.data());

        ImGui::Separator();

        const auto pipelineStats = Renderer::GetPipelineStatistics();
        for (auto& pipelineStat : pipelineStats)
            ImGui::Text("%s", pipelineStat.data());

        ImGui::TreePop();
    }

    ImGui::End();

    ImGui::Begin("Renderer Outputs");
    for (const auto& rendererOuput : Renderer::GetRendererOutput())
    {
        ImGui::Text(rendererOuput.Name.data());
        ImGui::Image(rendererOuput.Attachment->GetTextureID(),
                     {ImGui::GetContentRegionAvail().x * 0.75f, ImGui::GetContentRegionAvail().x * 0.75f});
        ImGui::Separator();
    }
    ImGui::End();

    m_SceneHierarchyPanel.OnImGuiRender();

    m_ContentBrowserPanel.OnImGuiRender();

    EndDockspace();
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

    if (!filePath.empty()) OpenScene(filePath);
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
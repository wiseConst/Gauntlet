#include "RuntimeLayer.h"
#include "Gauntlet/Scene/SceneSerializer.h"
#include "Gauntlet/Utils/PlatformUtils.h"

#define SPONZA_TEST 1

RuntimeLayer::RuntimeLayer() : Layer("RuntimeLayer"), m_GUILayer(Application::Get().GetGUILayer()) {}

void RuntimeLayer::OnAttach()
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
}

void RuntimeLayer::OnDetach()
{
    SceneSerializer serializer(m_ActiveScene);
    serializer.Serialize("Resources/Scenes/" + m_ActiveScene->GetName() + ".gntlt");
}

void RuntimeLayer::OnUpdate(const float deltaTime)
{
    if (m_bIsViewportFocused) m_EditorCamera->OnUpdate(deltaTime);

    Renderer::BeginScene(*m_EditorCamera);

    m_ActiveScene->OnUpdate(deltaTime);

    Renderer::EndScene();
}

void RuntimeLayer::OnEvent(Event& event)
{
    if (m_bIsViewportHovered) m_EditorCamera->OnEvent(event);
}

void RuntimeLayer::OnImGuiRender()
{
    UpdateViewportSize();

    BeginDockspace();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::Begin("FinalImage");
    m_ViewportSize = ImGui::GetContentRegionAvail();

    m_bIsViewportFocused = ImGui::IsWindowFocused();
    m_bIsViewportHovered = ImGui::IsWindowHovered();
    m_GUILayer->BlockEvents(m_bIsViewportFocused && !m_bIsViewportHovered);

    ImGui::Image(Renderer::GetFinalImage()->GetTextureID(), m_ViewportSize);
    ImGui::End();

    ImGui::PopStyleVar(2);

    EndDockspace();
}

void RuntimeLayer::BeginDockspace()
{
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_AutoHideTabBar;
    static bool bShowDockSpace                = true;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

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
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }
}

void RuntimeLayer::EndDockspace()
{
    ImGui::End();
}

void RuntimeLayer::UpdateViewportSize()
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
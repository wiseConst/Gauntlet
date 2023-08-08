#include "EditorLayer.h"

EditorLayer::EditorLayer() : Layer("EditorLayer"), m_GUILayer(Application::Get().GetGUILayer()) {}

void EditorLayer::OnAttach()
{
    m_EditorCamera = EditorCamera::Create();

    const float LoadMeshStartTime = Application::Get().GetTimeNow();
    m_CerberusMesh                = Mesh::Create(std::string(ASSETS_PATH) + "Models/Cerberus/scene.gltf");
    m_OmenMesh                    = Mesh::Create(std::string(ASSETS_PATH) + "Models/omen/scene.gltf");
    m_CyberPunkRevolverMesh       = Mesh::Create(std::string(ASSETS_PATH) + "Models/cyberpunk_revolver/scene.gltf");
    const float LoadMeshEndTime   = Application::Get().GetTimeNow();
    LOG_INFO("Time took to load meshes: (%f)ms", LoadMeshEndTime - LoadMeshStartTime);
}

void EditorLayer::OnDetach()
{
    m_CerberusMesh->Destroy();
    m_OmenMesh->Destroy();
    m_CyberPunkRevolverMesh->Destroy();
}

void EditorLayer::OnUpdate(const float DeltaTime)
{
    m_EditorCamera->OnUpdate(DeltaTime);

    Renderer::BeginScene(*m_EditorCamera);
    // Renderer2D::BeginScene(*m_EditorCamera);

    {
        const auto Translation = glm::translate(glm::mat4(1.0f), glm::vec3(0, -5.0f, 0.0f));
        const auto Rotation    = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1, 0, 0));
        const auto Scale       = glm::scale(glm::mat4(1.0f), glm::vec3(0.25f));
        const auto TRS         = Translation * Rotation * Scale;

        Renderer::SubmitMesh(m_CerberusMesh, TRS);
    }

    {
        const auto Translation = glm::translate(glm::mat4(1.0f), glm::vec3(-10.0f, -10.0f, 0.0f));
        const auto Rotation    = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(1, 0, 0));
        const auto Scale       = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f));
        const auto TRS         = Translation * Rotation * Scale;

        Renderer::SubmitMesh(m_OmenMesh, TRS);
    }

    {
        static float RotationAngle = 0.0f;
        RotationAngle += DeltaTime * 4;

        const auto Translation = glm::translate(glm::mat4(1.0f), glm::vec3(15.0f, 15.0f, 0.0f));
        const auto Rotation    = glm::rotate(glm::mat4(1.0f), glm::radians(RotationAngle), glm::vec3(0, 0, 1));
        const auto Scale       = glm::scale(glm::mat4(1.0f), glm::vec3(0.05f, 0.1f, 0.1f));
        const auto TRS         = Translation * Rotation * Scale;

        Renderer::SubmitMesh(m_CyberPunkRevolverMesh, TRS);
    }
    // Renderer2D::DrawQuad(glm::vec3(0.0f, -50.0f, 0.0f), glm::vec2(80), glm::vec4(1.0f, 1.0f, 0.5f, 1.0f));
}

void EditorLayer::OnEvent(Event& InEvent)
{
    m_EditorCamera->OnEvent(InEvent);
}

void EditorLayer::OnImGuiRender()
{
    UpdateViewportSize();

    BeginDockspace();

    static bool bShowAppStats = true;
    if (bShowAppStats)
    {
        ImGui::Begin("Application Stats", &bShowAppStats);

        const auto& Stats = Renderer::GetStats();
        ImGui::Text("Allocated Images: %llu", Stats.AllocatedImages);
        ImGui::Text("Allocated Buffers: %llu", Stats.AllocatedBuffers);
        ImGui::Text("Allocated StagingVertexBuffers: %llu", Stats.StagingVertexBuffers);
        ImGui::Text("VRAM Usage: %f MB", Stats.GPUMemoryAllocated / 1024.0f / 1024.0f);
        ImGui::Text("CPU Wait Time: %f(ms)", Stats.CPUWaitTime);
        ImGui::Text("GPU Wait Time: %f(ms)", Stats.GPUWaitTime);
        ImGui::Text("VMA Allocations: %llu", Stats.Allocations);
        ImGui::Text("DrawCalls: %llu", Stats.DrawCalls);
        ImGui::Text("QuadCount: %llu", Stats.QuadCount);

        ImGui::End();
    }

    ImGui::Begin("Hierarchy Panel");
    ImGui::Text("Entities");
    ImGui::End();

    ImGui::Begin("Content Browser");
    ImGui::Text("Files");
    ImGui::End();

    EndDockspace();
}

void EditorLayer::BeginDockspace()
{
    static bool opt_fullscreen                = true;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
    static bool bShowDockSpace                = true;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }
    else
    {
        dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
    }

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
    ImGui::PopStyleVar();

    if (opt_fullscreen) ImGui::PopStyleVar(2);

    // Submit the DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Viewport Settings"))
        {
            // Disabling fullscreen would allow the window to be moved to the front of other windows,
            // which we can't undo at the moment without finer window depth/z control.
            ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen);
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void EditorLayer::EndDockspace()
{
    ImGui::End();
}

void EditorLayer::UpdateViewportSize()
{
    const auto ViewportSize = m_GUILayer->GetViewportSize();
    const auto bNeedViewportResize =
        m_EditorCamera->GetViewportWidth() != ViewportSize.x || m_EditorCamera->GetViewportHeight() != ViewportSize.y;
    const auto bIsViewportValid = ViewportSize.x > 0.0f && ViewportSize.y > 0.0f;

    if (bNeedViewportResize && bIsViewportValid) m_EditorCamera->Resize(ViewportSize.x, ViewportSize.y);
}
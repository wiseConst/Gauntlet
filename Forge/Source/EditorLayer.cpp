#include "EditorLayer.h"
#include "Gauntlet/Scene/SceneSerializer.h"
#include "Gauntlet/Utils/PlatformUtils.h"

#define SPONZA_TEST 1
#define LOAD_TEST_BED 0
#define MAKE_TEST_BED 0

EditorLayer::EditorLayer() : Layer("EditorLayer"), m_GUILayer(Application::Get().GetGUILayer()) {}

void EditorLayer::OnAttach()
{
    m_EditorCamera = EditorCamera::Create();

#if LOAD_TEST_BED
    SceneSerializer serializer(m_ActiveScene);
    GNT_ASSERT(serializer.Deserialize("Resources/Scenes/TestBed.gntlt"), "Failed to deserialize scene!");
#endif

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

#if MAKE_TEST_BED
    m_ActiveScene = MakeRef<Scene>("TestBed");

    {
        auto Cerberus = m_ActiveScene->CreateEntity("Cerberus");
        Cerberus.AddComponent<MeshComponent>(Mesh::Create("Resources/Models/Cerberus/scene.gltf"));

        auto& tc       = Cerberus.GetComponent<TransformComponent>();
        tc.Translation = glm::vec3(0, -5.0f, 10.0f);
        tc.Scale       = glm::vec3(0.1f);
    }

    {
        auto CyberPunkRevolver = m_ActiveScene->CreateEntity("CyberPunkRevolver");
        CyberPunkRevolver.AddComponent<MeshComponent>(Mesh::Create("Resources/Models/cyberpunk_revolver/scene.gltf"));
        auto& tc = CyberPunkRevolver.GetComponent<TransformComponent>();

        tc.Translation = glm::vec3(55.0f, 55.0f, 0.0f);
        tc.Scale       = glm::vec3(0.05f);
    }

    {
        auto VikingRoom = m_ActiveScene->CreateEntity("VikingRoom");
        VikingRoom.AddComponent<MeshComponent>(Mesh::Create("Resources/Models/VikingRoom/viking_room.gltf"));
        auto& tc = VikingRoom.GetComponent<TransformComponent>();

        tc.Translation = glm::vec3(-10.0f, -10.0f, 0.0f);
        tc.Scale       = glm::vec3(4.0f);
    }

    {
        auto Sphere = m_ActiveScene->CreateEntity("Sphere");
        Sphere.AddComponent<MeshComponent>(Mesh::Create("Resources/Models/Default/Sphere.gltf"));
        auto& tc = Sphere.GetComponent<TransformComponent>();

        tc.Scale         = glm::vec3(150.0f);
        tc.Translation.y = 5.0f;
    }

    {
        auto Plane = m_ActiveScene->CreateEntity("Plane");
        Plane.AddComponent<MeshComponent>(Mesh::Create("Resources/Models/Default/Plane.gltf"));
        auto& tc = Plane.GetComponent<TransformComponent>();

        tc.Translation = glm::vec3(0.0f, -10.0f, 0.0f);
        tc.Scale       = glm::vec3(4.0f, 1.0f, 4.0f);
    }

    {
        auto rhetorician = m_ActiveScene->CreateEntity("rhetorician");
        rhetorician.AddComponent<MeshComponent>(Mesh::Create("Resources/Models/rhetorician/scene.gltf"));
        auto& tc = rhetorician.GetComponent<TransformComponent>();

        tc.Scale       = glm::vec3(2.0f);
        tc.Translation = glm::vec3(0.0f, 0.0f, -40.0f);
    }

    {
        auto handgun_animation = m_ActiveScene->CreateEntity("handgun_animation");
        handgun_animation.AddComponent<MeshComponent>(Mesh::Create("Resources/Models/handgun_animation/scene.gltf"));
        auto& tc = handgun_animation.GetComponent<TransformComponent>();

        tc.Scale       = glm::vec3(15.0f);
        tc.Translation = glm::vec3(-5.0f, 0.0f, -5.0f);
    }

    {
        auto PointLight = m_ActiveScene->CreateEntity("PointLight");
        PointLight.AddComponent<SpriteRendererComponent>(glm::vec4{0.2f});

        auto& tc       = PointLight.GetComponent<TransformComponent>();
        tc.Scale       = glm::vec3(1.5f);
        tc.Translation = glm::vec3(10.0f, 10.0f, -10.0f);

        auto& plc                      = PointLight.AddComponent<PointLightComponent>();
        plc.Color                      = glm::vec4(0.2f);
        plc.AmbientSpecularShininess.x = 0.1f;
        plc.AmbientSpecularShininess.y = 0.5f;
        plc.AmbientSpecularShininess.z = 32.0f;
        plc.CLQ                        = glm::vec3(1.0f, 0.004f, 0.00007f);
    }

    {
        auto DirectionalLight = m_ActiveScene->CreateEntity("DirectionalLight");
        DirectionalLight.AddComponent<SpriteRendererComponent>(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));

        auto& tc       = DirectionalLight.GetComponent<TransformComponent>();
        tc.Scale       = glm::vec3(2.5f);
        tc.Translation = glm::vec3(20.0f, 20.0f, -10.0f);

        auto& dlc                      = DirectionalLight.AddComponent<DirectionalLightComponent>();
        dlc.Color                      = glm::vec3(0.0f, 0.0f, 0.05f);
        dlc.Direction                  = tc.Translation;
        dlc.AmbientSpecularShininess.x = 0.25f;
        dlc.AmbientSpecularShininess.y = 0.6f;
        dlc.AmbientSpecularShininess.z = 32.0f;
    }
#endif

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

    Renderer::BeginScene(*m_EditorCamera);

    m_ActiveScene->OnUpdate(deltaTime);

    Renderer::EndScene();
}

void EditorLayer::OnEvent(Event& event)
{
    if (m_bIsViewportHovered) m_EditorCamera->OnEvent(event);
}

void EditorLayer::OnImGuiRender()
{
    UpdateViewportSize();

    BeginDockspace();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});

    ImGui::Begin("Viewport");
    m_ViewportSize = ImGui::GetContentRegionAvail();

    m_bIsViewportFocused = ImGui::IsWindowFocused();
    m_bIsViewportHovered = ImGui::IsWindowHovered();
    m_GUILayer->BlockEvents(m_bIsViewportFocused && !m_bIsViewportHovered);

    ImGui::Image(Renderer::GetFinalImage()->GetTextureID(), m_ViewportSize);
    ImGui::End();

    ImGui::PopStyleVar();

    /*static bool ShowDemoWindow = true;
    if (ShowDemoWindow) ImGui::ShowDemoWindow(&ShowDemoWindow);*/

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

    m_SceneHierarchyPanel.OnImGuiRender();

    ImGui::Begin("Content Browser");
    ImGui::Text("Files");
    ImGui::End();

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
            if (ImGui::MenuItem("New", "Ctrl+Shift+N"))
            {
                m_ActiveScene = MakeRef<Scene>();
                m_SceneHierarchyPanel.SetContext(m_ActiveScene);
            }

            if (ImGui::MenuItem("Open ...", "Ctrl+O"))
            {
                const std::string filePath = FileDialogs::OpenFile("Gauntlet Scene (*.gntlt)\0*.gntlt\0");

                if (filePath.empty())
                {
                    LOG_WARN("Failed to open scene: %s", filePath.data());
                }
                else
                {
                    m_ActiveScene = {};

                    SceneSerializer serializer(m_ActiveScene);
                    serializer.Deserialize(filePath);

                    m_SceneHierarchyPanel.SetContext(m_ActiveScene);
                }
            }

            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
            {
                const std::string filePath = FileDialogs::SaveFile("Gauntlet Scene (*.gntlt)\0*.gntlt\0");

                if (filePath.empty())
                {
                    LOG_WARN("Failed to save scene: %s", filePath.data());
                }
                else
                {
                    SceneSerializer serializer(m_ActiveScene);
                    serializer.Serialize(filePath);
                }
            }

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
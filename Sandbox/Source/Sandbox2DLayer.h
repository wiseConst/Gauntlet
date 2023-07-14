#pragma once

#include <Eclipse.h>

using namespace Eclipse;

class Sandbox2DLayer final : public Layer
{
  public:
    Sandbox2DLayer() : Layer("Sandbox2DLayer") {}
    ~Sandbox2DLayer() = default;

    void OnAttach() final override { m_Camera = OrthographicCamera::Create(); }
    void OnDetach() final override {}

    void OnUpdate(const float DeltaTime) final override
    {
        Renderer2D::SetClearColor({0.1f, 0.1f, 0.1f, 1.0f});

        m_Camera->OnUpdate(DeltaTime);
        Renderer2D::BeginScene(*m_Camera, true);

        for (uint32_t i = 0; i < 100; ++i)
        {
            for (uint32_t j = 0; j < 100; ++j)
            {
                Renderer2D::DrawQuad({i, j}, {0.45f, 0.45f}, glm::vec4(1.0f));
            }
        }
    }

    void OnEvent(Event& InEvent) final override { m_Camera->OnEvent(InEvent); }

    void OnImGuiRender() final override
    {
#if 0
        static bool opt_fullscreen                = true;
        static bool opt_padding                   = false;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
        static bool bShowDockSpace                = true;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
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
        if (!opt_padding) ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", &bShowDockSpace, window_flags);
        if (!opt_padding) ImGui::PopStyleVar();

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
                ImGui::MenuItem("Padding", NULL, &opt_padding);
                ImGui::Separator();

                if (ImGui::MenuItem("Flag: NoSplit", "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0))
                {
                    dockspace_flags ^= ImGuiDockNodeFlags_NoSplit;
                }
                if (ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0))
                {
                    dockspace_flags ^= ImGuiDockNodeFlags_NoResize;
                }
                if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))
                {
                    dockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode;
                }
                if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))
                {
                    dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
                }
                if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0,
                                    opt_fullscreen))
                {
                    dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode;
                }
                ImGui::Separator();

                if (ImGui::MenuItem("Close", NULL, false)) bShowDockSpace = false;
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
#endif
        static bool bShowAppStats = true;
        if (bShowAppStats)
        {
            ImGui::Begin("Application Stats", &bShowAppStats);

            const auto& Stats = Renderer2D::GetStats();
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
#if 0
        ImGui::End();
#endif
    }

  private:
    Ref<OrthographicCamera> m_Camera;
};
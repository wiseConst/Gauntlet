#pragma once

#include <Gauntlet.h>

using namespace Gauntlet;

class Sandbox2DLayer final : public Layer
{
  public:
    Sandbox2DLayer() : Layer("Sandbox2DLayer") {}
    ~Sandbox2DLayer() = default;

    void OnAttach() final override { m_Camera = PerspectiveCamera::Create(); }

    void OnDetach() final override {}

    void OnUpdate(const float DeltaTime) final override
    {
        /* Renderer2D::BeginScene(*m_Camera, true);

         for (int32_t i = -10; i < 10; ++i)
         {
             for (int32_t j = -10; j < 10; ++j)
             {
                 if (i * i + j * j < 45) Renderer2D::DrawQuad({i, j, -5.0f}, {0.45f, 0.45f}, glm::vec4(1.0f));
             }
         }*/
    }

    void OnEvent(Event& InEvent) final override { m_Camera->OnEvent(InEvent); }

    void OnImGuiRender() final override
    {
        static bool bShowAppStats = true;
        if (bShowAppStats)
        {
            /*ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoResize;
            ImGui::SetNextWindowPos(ImVec2(5, 5));
            ImGui::SetNextWindowSize(ImVec2(0, 0));*/
            ImGui::Begin("Application Stats", &bShowAppStats /*, WindowFlags*/);

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
    }

  private:
    Ref<PerspectiveCamera> m_Camera;
};
#pragma once

#include "Gauntlet/Core/Core.h"
#include "Gauntlet/Layer/Layer.h"

#include <imgui/imgui.h>

#include <glm/glm.hpp>

namespace Gauntlet
{

class ImGuiLayer : public Layer
{
  public:
    ImGuiLayer(const std::string_view& DebugName) : Layer(DebugName) {}
    ImGuiLayer()          = default;
    virtual ~ImGuiLayer() = default;

    virtual void OnUpdate(const float DeltaTime) = 0;
    virtual void OnEvent(Event& event)         = 0;
    virtual void OnImGuiRender()                 = 0;

    virtual void OnAttach() = 0;
    virtual void OnDetach() = 0;

    virtual void BeginRender() = 0;
    virtual void EndRender()   = 0;
    virtual void BlockEvents(const bool bBlockEvents) = 0;

    static Scoped<ImGuiLayer> Create();

};

}  // namespace Gauntlet
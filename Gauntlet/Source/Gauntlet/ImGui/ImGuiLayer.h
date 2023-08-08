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
    ImGuiLayer(const std::string_view& InDebugName) : Layer(InDebugName) {}
    ImGuiLayer()          = default;
    virtual ~ImGuiLayer() = default;

    virtual void OnUpdate(const float DeltaTime) {}
    virtual void OnEvent(Event& InEvent) {}
    virtual void OnImGuiRender() {}

    virtual void OnAttach() = 0;
    virtual void OnDetach() = 0;

    virtual void BeginRender() = 0;
    virtual void EndRender()   = 0;

    // To be removed
    FORCEINLINE virtual const glm::vec2 GetViewportSize() const = 0;

    static ImGuiLayer* Create();
};

}  // namespace Gauntlet
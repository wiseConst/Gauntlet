#pragma once

#include "Eclipse/Core/Core.h"
#include "Eclipse/Layer/Layer.h"

#include <imgui/imgui.h>

#include <glm/glm.hpp>

namespace Eclipse
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

#if ELS_EDITOR
    FORCEINLINE virtual const glm::vec2 GetViewportSize() const = 0;
#endif

    static ImGuiLayer* Create();
};

}  // namespace Eclipse
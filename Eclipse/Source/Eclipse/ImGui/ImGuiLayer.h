#pragma once

#include "Eclipse/Core/Core.h"
#include "Eclipse/Layer/Layer.h"

#include <imgui.h>

namespace Eclipse
{

class ImGuiLayer : public Layer, private Uncopyable, private Unmovable
{
  public:
    ImGuiLayer(const std::string_view& InDebugName) : Layer(InDebugName) {}
    ImGuiLayer() = default;
    virtual ~ImGuiLayer() = default;

    virtual void OnUpdate(const float DeltaTime){};

    virtual void OnAttach() = 0;
    virtual void OnDetach() = 0;

    virtual void BeginRender() = 0;
    virtual void EndRender() = 0;

    static ImGuiLayer* Create();
};

}  // namespace Eclipse
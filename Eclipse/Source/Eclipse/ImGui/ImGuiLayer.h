#pragma once

#include "Eclipse/Core/Core.h"
#include "Eclipse/Layer/Layer.h"

namespace Eclipse
{

class ImGuiLayer : public Layer, private Uncopyable, private Unmovable
{
  public:
    ImGuiLayer() = default;
    virtual ~ImGuiLayer() = default;

    virtual void OnUpdate(const float DeltaTime) = 0;

    virtual void OnAttach() = 0;
    virtual void OnDetach() = 0;
};

}  // namespace Eclipse
#pragma once

#include "Gauntlet/Core/Core.h"
#include "Gauntlet/Event/Event.h"

namespace Gauntlet
{
class Layer : private Uncopyable, private Unmovable
{
  public:
    Layer() = default;
    Layer(const std::string_view& layerName) : m_LayerName(layerName) {}
    virtual ~Layer() = default;

    virtual void OnUpdate(const float deltaTime) = 0;
    virtual void OnEvent(Event& event)           = 0;

    virtual void OnImGuiRender() = 0;

    virtual void OnAttach() = 0;
    virtual void OnDetach() = 0;

  private:
    std::string_view m_LayerName;
};
}  // namespace Gauntlet
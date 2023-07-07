#pragma once

#include "Eclipse/Core/Core.h"
#include "Eclipse/Event/Event.h"

namespace Eclipse
{
class Layer : private Uncopyable, private Unmovable
{
  public:
    Layer() = default;
    Layer(const std::string_view& InLayerName) : m_LayerName(InLayerName) {}
    virtual ~Layer() = default;

    virtual void OnUpdate(const float DeltaTime) = 0;
    virtual void OnEvent(Event& InEvent) = 0;

    virtual void OnAttach() = 0;
    virtual void OnDetach() = 0;

  private:
    std::string_view m_LayerName;
};
}  // namespace Eclipse
#pragma once

#include "Event.h"

namespace Gauntlet
{
class WindowResizeEvent final : public Event
{
  public:
    WindowResizeEvent(int width, int height) : Event("WindowResizeEvent", EEventType::WindowResizeEvent), m_Width(width), m_Height(height)
    {
    }

    virtual std::string Format() const override final
    {
        std::string formatted = m_Name + ": (" + std::to_string(m_Width) + ", " + std::to_string(m_Height) + ")";
        return formatted;
    }

    FORCEINLINE const float GetAspectRatio() const { return m_Width / (float)m_Height; }

    FORCEINLINE const auto GetHeight() const { return m_Height; }
    FORCEINLINE const auto GetWidth() const { return m_Width; }

    EVENT_CLASS_TYPE(WindowResizeEvent)

  private:
    int m_Width;
    int m_Height;
};

class WindowCloseEvent final : public Event
{
  public:
    WindowCloseEvent() : Event("WindowCloseEvent", EEventType::WindowCloseEvent) {}

    virtual std::string Format() const override final { return m_Name; }

    EVENT_CLASS_TYPE(WindowCloseEvent)
};

}  // namespace Gauntlet
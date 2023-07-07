#pragma once

#include "Event.h"

namespace Eclipse
{

class MouseMovedEvent final : public Event
{
  public:
    MouseMovedEvent(int x, int y) : Event("MouseMovedEvent", EEventType::MouseMovedEvent), m_X(x), m_Y(y) {}

    virtual std::string Format() const override final
    {
        std::string formatted = m_Name + ": (" + std::to_string(m_X) + ", " + std::to_string(m_Y) + ")";
        return formatted;
    }

  private:
    int m_X;
    int m_Y;
};

class MouseScrolledEvent final : public Event
{
  public:
    MouseScrolledEvent(int xoffset, int yoffset)
        : Event("MouseScrolledEvent", EEventType::MouseScrolledEvent), m_XOffset(xoffset), m_YOffset(yoffset)
    {
    }

    virtual std::string Format() const override final
    {
        std::string formatted = m_Name + ": (" + std::to_string(m_XOffset) + ", " + std::to_string(m_XOffset) + ")";
        return formatted;
    }

    FORCEINLINE const auto GetOffsetX() const { return m_XOffset; }
    FORCEINLINE const auto GetOffsetY() const { return m_YOffset; }

    FORCEINLINE auto GetOffsetX() { return m_XOffset; }
    FORCEINLINE auto GetOffsetY() { return m_YOffset; }

  private:
    int m_XOffset;
    int m_YOffset;
};

class MouseButtonPressedEvent final : public Event
{
  public:
    MouseButtonPressedEvent(int button, int action)
        : Event("MouseButtonPressedEvent", EEventType::MouseButtonPressedEvent), m_Button(button), m_Action(action)
    {
    }

    virtual std::string Format() const override final
    {
        std::string formatted = m_Name + ": (" + std::to_string(m_Button) + ", " + std::to_string(m_Action) + ")";
        return formatted;
    }

  private:
    int m_Button;
    int m_Action;
};

class MouseButtonReleasedEvent final : public Event
{
  public:
    MouseButtonReleasedEvent(int button, int action)
        : Event("MouseButtonReleasedEvent", EEventType::MouseButtonReleasedEvent), m_Button(button), m_Action(action)
    {
    }

    virtual std::string Format() const override final
    {
        std::string formatted = m_Name + ": (" + std::to_string(m_Button) + ", " + std::to_string(m_Action) + ")";
        return formatted;
    }

  private:
    int m_Button;
    int m_Action;
};

class MouseButtonRepeatedEvent final : public Event
{
  public:
    MouseButtonRepeatedEvent(int button, int action, int count)
        : Event("MouseButtonRepeatedEvent", EEventType::MouseButtonRepeatedEvent), m_Button(button), m_Action(action), m_Count(count)
    {
    }

    virtual std::string Format() const override final
    {
        std::string formatted =
            m_Name + ": (" + std::to_string(m_Count) + ") - (" + std::to_string(m_Button) + ", " + std::to_string(m_Action) + ")";
        return formatted;
    }

  private:
    int m_Button;
    int m_Action;
    int m_Count;
};

}  // namespace Eclipse

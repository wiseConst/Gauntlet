#pragma once

#include <GauntletPCH.h>
#include <Gauntlet/Core/Core.h>

// From winderton

namespace Gauntlet
{
enum class EEventType : uint8_t
{
    MouseMovedEvent = 0,
    MouseScrolledEvent,
    MouseButtonPressedEvent,
    MouseButtonReleasedEvent,
    MouseButtonRepeatedEvent,

    KeyButtonPressedEvent,
    KeyButtonReleasedEvent,
    KeyButtonRepeatedEvent,

    WindowResizeEvent,
    WindowCloseEvent
};

#define EVENT_CLASS_TYPE(type)                                                                                                             \
    FORCEINLINE static EEventType GetStaticType()                                                                                          \
    {                                                                                                                                      \
        return EEventType::type;                                                                                                           \
    }

class EventDispatcher;

class Event
{
  public:
    Event()          = delete;
    virtual ~Event() = default;

    Event(const Event& e)  = delete;
    Event(const Event&& e) = delete;

    Event& operator=(const Event& e) = delete;
    Event& operator=(Event&& e)      = delete;

    FORCEINLINE const auto& GetName() const { return m_Name; }
    FORCEINLINE const auto& GetType() const { return m_Type; }

    virtual std::string Format() const = 0;

    FORCEINLINE const bool IsHandled() const { return m_bIsHandled; }
    FORCEINLINE void SetHandled(const bool bIsHandled) { m_bIsHandled = bIsHandled; }

  protected:
    Event(const std::string& name, EEventType type) : m_Name(name), m_Type(type), m_bIsHandled(false) {}

    EEventType m_Type;
    std::string m_Name;
    bool m_bIsHandled;
};

// Event Dispatcher

class EventDispatcher final : private Uncopyable, private Unmovable
{
  public:
    EventDispatcher(Event& event) : m_Event(event) {}

    template <typename T, typename F> void Dispatch(const F& func)
    {
        if (m_Event.GetType() == T::GetStaticType())
        {
            func(static_cast<T&>(m_Event));
            m_Event.SetHandled(true);
        }
    }

  private:
    Event& m_Event;
};

}  // namespace Gauntlet
#pragma once

#include "Event.h"

namespace Gauntlet
{
class KeyButtonPressedEvent final : public Event
{
  public:
    KeyButtonPressedEvent(int32_t key, int32_t scancode, bool bIsRepeat = false)
        : Event("KeyButtonPressedEvent", EEventType::KeyButtonPressedEvent), m_Key(key), m_Scancode(scancode), m_bIsRepeat(bIsRepeat)
    {
    }

    FORCEINLINE bool IsRepeat() const { return m_bIsRepeat; }

    FORCEINLINE std::string Format() const override final
    {
        std::string formatted = m_Name + ": (" + std::to_string(m_Key) + ", " + std::to_string(m_Scancode) + ")";
        return formatted;
    }

    EVENT_CLASS_TYPE(KeyButtonPressedEvent)

  private:
    int32_t m_Key;
    int32_t m_Scancode;
    bool m_bIsRepeat;
};

class KeyButtonReleasedEvent final : public Event
{
  public:
    KeyButtonReleasedEvent(int key, int scancode)
        : Event("KeyButtonReleasedEvent", EEventType::KeyButtonReleasedEvent), m_Key(key), m_Scancode(scancode)
    {
    }

    FORCEINLINE std::string Format() const override final
    {
        std::string formatted = m_Name + ": (" + std::to_string(m_Key) + ", " + std::to_string(m_Scancode) + ")";
        return formatted;
    }

    EVENT_CLASS_TYPE(KeyButtonReleasedEvent)

  private:
    int m_Key;
    int m_Scancode;
};

}  // namespace Gauntlet
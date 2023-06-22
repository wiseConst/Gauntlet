#pragma once

#include "Event.h"

namespace Eclipse
{
	class KeyButtonPressedEvent final : public Event
	{
	public:
		KeyButtonPressedEvent(int key, int scancode) : Event("KeyButtonPressedEvent",EventType::KeyButtonPressedEvent),m_Key(key),m_Scancode(scancode)
		{
		}

		virtual std::string Format() const override final
		{
			std::string formatted = m_Name + ": (" + std::to_string(m_Key) + ", " + std::to_string(m_Scancode) + ")";
			return formatted;
		}
	private:
		int m_Key;
		int m_Scancode;
	};

	class KeyButtonReleasedEvent final : public Event
	{
	public:
		KeyButtonReleasedEvent(int key, int scancode) : Event("KeyButtonReleasedEvent", EventType::KeyButtonReleasedEvent), m_Key(key), m_Scancode(scancode)
		{
		}

		virtual std::string Format() const override final
		{
			std::string formatted = m_Name + ": (" + std::to_string(m_Key) + ", " + std::to_string(m_Scancode) + ")";
			return formatted;
		}
	private:
		int m_Key;
		int m_Scancode;
	};

	class KeyButtonRepeatedEvent final : public Event
	{
	public:
		KeyButtonRepeatedEvent(int key, int scancode, int action, int count) : Event("KeyButtonRepeatedEvent", EventType::KeyButtonRepeatedEvent), m_Key(key), m_Scancode(scancode), m_Action(action),m_Count(count)
		{
		}

		virtual std::string Format() const override final
		{
			std::string formatted = m_Name + ": (" + std::to_string(m_Key) + ", " + std::to_string(m_Scancode) + ", " + std::to_string(m_Action) +  ", " + std::to_string(m_Count) + ")";
			return formatted;
		}
	private:
		int m_Key;
		int m_Scancode;
		int m_Action;
		int m_Count;
	};
}
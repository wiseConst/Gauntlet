#pragma once

#include "Event.h"

namespace Eclipse
{

	class MouseMovedEvent final : public Event
	{
	public:
		MouseMovedEvent(int x, int y)
			:Event("MouseMovedEvent", EventType::MouseMovedEvent), m_X(x), m_Y(y)
		{

		}

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
			:Event("MouseScrolledEvent", EventType::MouseScrolledEvent), m_XOffset(xoffset), m_YOffset(yoffset)
		{

		}

		virtual std::string Format() const override final
		{
			std::string formatted = m_Name + ": (" + std::to_string(m_XOffset) + ", " + std::to_string(m_XOffset) + ")";
			return formatted;
		}

	private:
		int m_XOffset;
		int m_YOffset;
	};


	class MouseButtonPressedEvent final : public Event
	{
	public:
		MouseButtonPressedEvent(int button, int action)
			:Event("MouseButtonPressedEvent", EventType::MouseButtonPressedEvent), m_Button(button), m_Action(action)
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
			:Event("MouseButtonReleasedEvent", EventType::MouseButtonReleasedEvent), m_Button(button), m_Action(action)
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
			:Event("MouseButtonRepeatedEvent", EventType::MouseButtonRepeatedEvent), m_Button(button), m_Action(action),m_Count(count)
		{

		}

		virtual std::string Format() const override final
		{
			std::string formatted = m_Name + ": (" + std::to_string(m_Count) + ") - (" + std::to_string(m_Button) + ", " + std::to_string(m_Action) + ")";
			return formatted;
		}

	private:
		int m_Button;
		int m_Action;
		int m_Count;
	};

}

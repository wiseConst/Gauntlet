#pragma once

#include "Event.h"

namespace Eclipse
{
	class WindowResizeEvent final : public Event
	{
	public:
		WindowResizeEvent(int width, int height) : Event("WindowResizeEvent", EventType::WindowResizeEvent), m_Width(width), m_Height(height) {}


		virtual std::string Format() const override final
		{
			std::string formatted = m_Name + ": (" + std::to_string(m_Width) + ", " + std::to_string(m_Height) + ")";
			return formatted;
		}

	private:
		int m_Width;
		int m_Height;
	};

	class WindowCloseEvent final : public Event
	{
	public:
		WindowCloseEvent() : Event("WindowCloseEvent", EventType::WindowCloseEvent) {}

		virtual std::string Format() const override final
		{
			return m_Name;
		}
	};



}
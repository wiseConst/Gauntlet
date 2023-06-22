#pragma once

#include <EclipsePCH.h>
#include <Eclipse/Core/Core.h>

// From winderton

namespace Eclipse
{
	class Event
	{
	public:

		enum class EventType : uint8_t
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

		Event() = delete;
		virtual ~Event() = default;

		Event(const Event& e) = delete;
		Event(const Event&& e) = delete;

		Event& operator=(const Event& e) = delete;
		Event& operator=(Event&& e) = delete;

		FORCEINLINE const auto& GetName() const { return m_Name; }
		FORCEINLINE const auto& GetType() const { return m_Type; }

		virtual std::string Format() const = 0;
	protected:
		Event(const std::string& name, EventType type):m_Name(name),m_Type(type)
		{}

		EventType m_Type;
		std::string m_Name;
	};
}
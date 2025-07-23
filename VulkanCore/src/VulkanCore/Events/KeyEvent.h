#pragma once
#include "VulkanCore/Events/Event.h"

namespace VulkanCore {

	class KeyEvent : public Event
	{
	public:
		int GetKeyCode() const { return m_KeyCode; }

		EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
	public:
		KeyEvent(int keycode)
			: m_KeyCode(keycode) {}

		int m_KeyCode;
	};

	class KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent(int keycode, int repeatcount)
			: KeyEvent(keycode), m_RepeatCount(repeatcount) {}

		int GetRepeatCount() const { return m_RepeatCount; }

		std::string ToString() const override
		{
			return std::format("KeyPressedEvent: {0}({1} repeats)", m_KeyCode, m_RepeatCount);
		}

		EVENT_CLASS_TYPE(KeyPressed)
	private:
		int m_RepeatCount;
	};

	class KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent(int keycode)
			: KeyEvent(keycode) {}

		std::string ToString() const override
		{
			return std::format("KeyReleasedEvent: {}", m_KeyCode);
		}

		EVENT_CLASS_TYPE(KeyReleased)
	};

	class KeyTypedEvent : public KeyEvent
	{
	public:
		KeyTypedEvent(int keycode)
			: KeyEvent(keycode) {}

		std::string ToString() const override
		{
			return std::format("KeyTypedEvent: {}", m_KeyCode);
		}

		EVENT_CLASS_TYPE(KeyTyped)
	};

}

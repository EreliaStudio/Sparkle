#pragma once

#include <variant>

#include "core/event/event.hpp"

#include "core/window.hpp"
#include "input/keyboard.hpp"
#include "input/mouse.hpp"

namespace spk
{
	struct BaseEventRecord
	{
		Window::Identifier windowIdentifier;
	};

	struct WindowResizedRecord : public BaseEventRecord
	{
		spk::Vector2UInt size;
	};
	struct WindowMovedRecord : public BaseEventRecord
	{
	};
	struct WindowFocusGainedRecord : public BaseEventRecord
	{
	};
	struct WindowFocusLostRecord : public BaseEventRecord
	{
	};

	struct MouseEnteredRecord : public BaseEventRecord
	{
	};
	struct MouseLeftRecord : public BaseEventRecord
	{
	};
	struct MouseMovedRecord : public BaseEventRecord
	{
		spk::Vector2Int position;
	};
	struct MouseWheelScrolledRecord : public BaseEventRecord
	{
		spk::Vector2 value;
	};
	struct MouseButtonPressedRecord : public BaseEventRecord
	{
		spk::Mouse::Button button;
	};
	struct MouseButtonReleasedRecord : public BaseEventRecord
	{
		spk::Mouse::Button button;
	};
	struct MouseButtonDoubleClickedRecord : public BaseEventRecord
	{
		spk::Mouse::Button button;
	};

	struct KeyPressedRecord : public BaseEventRecord
	{
		spk::Keyboard::Key key;
	};
	struct KeyReleasedRecord : public BaseEventRecord
	{
		spk::Keyboard::Key key;
	};
	struct TextInputRecord : public BaseEventRecord
	{
		char32_t glyph;
	};

	using EventRecord = std::variant<
		WindowResizedRecord,
		WindowMovedRecord,
		WindowFocusGainedRecord,
		WindowFocusLostRecord,

		MouseEnteredRecord,
		MouseLeftRecord,
		MouseMovedRecord,
		MouseWheelScrolledRecord,
		MouseButtonPressedRecord,
		MouseButtonReleasedRecord,
		MouseButtonDoubleClickedRecord,

		KeyPressedRecord,
		KeyReleasedRecord,
		TextInputRecord>;

	using WindowResizedEvent = Event<WindowResizedRecord>;
	using WindowMovedEvent = Event<WindowMovedRecord>;
	using WindowFocusGainedEvent = Event<WindowFocusGainedRecord>;
	using WindowFocusLostEvent = Event<WindowFocusLostRecord>;

	using MouseEnteredEvent = Event<MouseEnteredRecord>;
	using MouseLeftEvent = Event<MouseLeftRecord>;
	struct MouseMovedEvent final : public DeviceEvent<MouseMovedRecord, spk::Mouse>
	{
	private:
		std::optional<spk::Vector2Int> _mousePositionRequest;

	public:
		MouseMovedEvent(const MouseMovedRecord &record, const spk::Mouse &mouse);

		void requestMousePosition(const spk::Vector2Int &position) noexcept;
		[[nodiscard]] const std::optional<spk::Vector2Int> &mousePositionRequest() const noexcept;
	};
	using MouseWheelScrolledEvent = DeviceEvent<MouseWheelScrolledRecord, spk::Mouse>;
	using MouseButtonPressedEvent = DeviceEvent<MouseButtonPressedRecord, spk::Mouse>;
	using MouseButtonReleasedEvent = DeviceEvent<MouseButtonReleasedRecord, spk::Mouse>;
	using MouseButtonDoubleClickedEvent = DeviceEvent<MouseButtonDoubleClickedRecord, spk::Mouse>;

	using KeyPressedEvent = DeviceEvent<KeyPressedRecord, spk::Keyboard>;
	using KeyReleasedEvent = DeviceEvent<KeyReleasedRecord, spk::Keyboard>;
	using TextInputEvent = DeviceEvent<TextInputRecord, spk::Keyboard>;
}

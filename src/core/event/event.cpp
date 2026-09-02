#include "core/event/record.hpp"

#include <cstddef>

namespace spk
{
	bool EventBase::_setFocusChange(FocusMode::Channel channel, FocusMode::ChangeType type, Widget *widget)
	{
		auto &change = _focusRecords[static_cast<std::size_t>(channel)];
		if (change.has_value())
		{
			if (type != FocusMode::ChangeType::Take)
			{
				return false;
			}
			change = FocusMode::Record{.type = type, .widget = widget};
			return true;
		}
		change = FocusMode::Record{.type = type, .widget = widget};
		return true;
	}

	bool EventBase::takeFocus(FocusMode::Channel channel, Widget *widget)
	{
		return _setFocusChange(channel, FocusMode::ChangeType::Take, widget);
	}

	bool EventBase::releaseFocus(FocusMode::Channel channel, Widget *widget)
	{
		return _setFocusChange(channel, FocusMode::ChangeType::Release, widget);
	}

	const std::optional<FocusMode::Record> &EventBase::focusChange(FocusMode::Channel channel) const
	{
		return _focusRecords[static_cast<std::size_t>(channel)];
	}

	MouseMovedEvent::MouseMovedEvent(const MouseMovedRecord &record, const spk::Mouse &mouse) :
		DeviceEvent(record, mouse)
	{
	}

	void MouseMovedEvent::requestMousePosition(const spk::Vector2Int &position) noexcept
	{
		_mousePositionRequest = position;
	}

	const std::optional<spk::Vector2Int> &MouseMovedEvent::mousePositionRequest() const noexcept
	{
		return _mousePositionRequest;
	}
}

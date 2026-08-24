#include "core/event/event.hpp"

#include <cstddef>

namespace spk
{
	bool EventBase::_setFocusChange(FocusMode::Channel channel, FocusMode::ChangeType type, Widget *widget)
	{
		auto &change = _focusRecords[static_cast<std::size_t>(channel)];
		if (change.has_value())
		{
			return false;
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
}

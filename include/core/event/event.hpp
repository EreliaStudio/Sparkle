#pragma once

#include <array>
#include <optional>

#include "type/focus_mode.hpp"

namespace spk
{
	struct EventBase
	{
	private:
		std::array<std::optional<FocusMode::Record>, FocusMode::ChannelCount> _focusRecords;
		bool _setFocusChange(FocusMode::Channel channel, FocusMode::ChangeType type, Widget *widget);

	public:
		bool consumed = false;

		// A successful Take returns true even when the same widget already has
		// the channel's pending focus request. A Take may replace an earlier
		// request in that channel; requests in other channels remain unchanged.
		bool takeFocus(FocusMode::Channel channel, Widget *widget);
		bool releaseFocus(FocusMode::Channel channel, Widget *widget);
		[[nodiscard]] const std::optional<FocusMode::Record> &focusChange(FocusMode::Channel channel) const;
	};

	template <typename TRecordType>
	struct Event : public EventBase
	{
		const TRecordType &record;

		explicit Event(const TRecordType &record) :
			record(record)
		{
		}
	};

	template <typename TRecordType, typename TDevice>
	struct DeviceEvent : public Event<TRecordType>
	{
		const TDevice &device;

		DeviceEvent(const TRecordType &record, const TDevice &device) :
			Event<TRecordType>(record),
			device(device)
		{
		}
	};
}

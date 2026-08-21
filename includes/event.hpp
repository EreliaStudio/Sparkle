#pragma once

#include <array>
#include <optional>

#include "focus_mode.hpp"

namespace spk
{
	struct EventBase
	{
	private:
		std::array<std::optional<FocusMode::Record>, FocusMode::ChannelCount> _focusRecords;
		bool _setFocusChange(FocusMode::Channel channel, FocusMode::ChangeType type, Widget *widget);

	public:
		bool consumed = false;

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

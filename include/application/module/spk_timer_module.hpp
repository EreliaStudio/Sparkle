#pragma once

#include "application/module/spk_module.hpp"

#include "structure/system/event/spk_event.hpp"

namespace spk
{
	class TimerModule : public spk::Module<spk::TimerEvent>
	{
	private:
		static const int UpdaterIdentifier = 1;

		void _treatEvent(spk::TimerEvent p_event) override;
		spk::TimerEvent _convertEventToEventType(spk::Event p_event) override;

	public:
		TimerModule() = default;
	};
}
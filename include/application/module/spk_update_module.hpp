#pragma once

#include "application/module/spk_module.hpp"

#include "structure/system/event/spk_event.hpp"

namespace spk
{
	class UpdateModule : public spk::Module<spk::UpdateEvent>
	{
	private:
		long long _lastTime = 0;

		void _treatEvent(spk::UpdateEvent&& p_event) override;
		spk::UpdateEvent _convertEventToEventType(spk::Event&& p_event) override;

	public:
		UpdateModule();
	};
}
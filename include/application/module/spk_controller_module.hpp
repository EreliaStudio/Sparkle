#pragma once

#include "application/module/spk_module.hpp"

#include "structure/system/event/spk_event.hpp"

namespace spk
{
	class ControllerModule : public spk::Module<spk::ControllerEvent>
	{
	private:
		spk::Controller _controller;

		void _treatEvent(spk::ControllerEvent&& p_event) override;
		spk::ControllerEvent _convertEventToEventType(spk::Event&& p_event) override;

	public:
		ControllerModule();

		const spk::Controller& controller() const;
	};
}
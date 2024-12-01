#pragma once

#include "application/module/spk_module.hpp"

#include "structure/system/event/spk_event.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{
	class ControllerModule : public spk::Module<spk::ControllerEvent>
	{
	private:
		spk::Widget* _rootWidget;

		spk::Controller _controller;

		void _treatEvent(spk::ControllerEvent&& p_event) override;
		spk::ControllerEvent _convertEventToEventType(spk::Event&& p_event) override;

	public:
		ControllerModule();

		void linkToWidget(spk::Widget* p_rootWidget);

		const spk::Controller& controller() const;
	};
}
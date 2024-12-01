#pragma once

#include "application/module/spk_module.hpp"

#include "structure/system/event/spk_event.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{
	class UpdateModule : public spk::Module<spk::UpdateEvent>
	{
	private:
		spk::Widget* _rootWidget;
		const spk::Mouse* _mouse;
		const spk::Keyboard* _keyboard;
		const spk::Controller* _controller;

		void _treatEvent(spk::UpdateEvent&& p_event) override;
		spk::UpdateEvent _convertEventToEventType(spk::Event&& p_event) override;

	public:
		UpdateModule();

		void linkToMouse(const spk::Mouse* p_mouse);
		void linkToKeyboard(const spk::Keyboard* p_keyboard);
		void linkToController(const spk::Controller* p_controller);

		void linkToWidget(spk::Widget* p_rootWidget);
	};
}
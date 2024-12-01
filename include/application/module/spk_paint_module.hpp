#pragma once

#include "application/module/spk_module.hpp"

#include "structure/system/event/spk_event.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{
	class PaintModule : public spk::Module<spk::PaintEvent>
	{
	private:
		spk::Widget* _rootWidget;

		void _treatEvent(spk::PaintEvent&& p_event) override;
		spk::PaintEvent _convertEventToEventType(spk::Event&& p_event) override;

	public:
		PaintModule();

		void linkToWidget(spk::Widget* p_rootWidget);
	};
}
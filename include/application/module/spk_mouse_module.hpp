#pragma once

#include "application/module/spk_module.hpp"

#include "structure/system/event/spk_event.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{
	class MouseModule : public spk::Module<spk::MouseEvent>
	{
	private:
		spk::Widget* _rootWidget;
		spk::Mouse _mouse;

		void _treatEvent(spk::MouseEvent&& p_event) override;
		spk::MouseEvent _convertEventToEventType(spk::Event&& p_event) override;

	public:
		MouseModule();

		void linkToWidget(spk::Widget* p_rootWidget);

		const spk::Mouse& mouse() const;
	};
}
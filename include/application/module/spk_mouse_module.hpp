#pragma once

#include "application/module/spk_module.hpp"

#include "structure/system/event/spk_event.hpp"

namespace spk
{
	class MouseModule : public spk::Module<spk::MouseEvent>
	{
	private:
		spk::Mouse _mouse;

		void _treatEvent(spk::MouseEvent p_event) override;
		spk::MouseEvent _convertEventToEventType(spk::Event p_event) override;

	public:
		MouseModule() = default;
		
		const spk::Mouse &mouse() const;
	};
}
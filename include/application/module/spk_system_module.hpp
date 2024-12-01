#pragma once

#include "application/module/spk_module.hpp"

#include "structure/system/event/spk_event.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{	
	class SystemModule : public spk::Module<spk::SystemEvent>
	{
	private:		
		spk::Geometry2D::Size newWindowSize;

		void _treatEvent(spk::SystemEvent&& p_event) override;
		spk::SystemEvent _convertEventToEventType(spk::Event&& p_event) override;

	public:
		SystemModule();
	};
}
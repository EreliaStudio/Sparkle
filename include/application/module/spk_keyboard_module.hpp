#pragma once

#include "application/module/spk_module.hpp"

#include "structure/system/event/spk_event.hpp"

namespace spk
{
	class KeyboardModule : public spk::Module<spk::KeyboardEvent>
	{
	private:
		spk::Keyboard _keyboard;

		void _treatEvent(spk::KeyboardEvent &&p_event) override;
		spk::KeyboardEvent _convertEventToEventType(spk::Event &&p_event) override;

	public:
		KeyboardModule();

		const spk::Keyboard &keyboard() const;
	};
}
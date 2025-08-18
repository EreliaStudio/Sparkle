#pragma once

#include "application/module/spk_module.hpp"
#include "structure/system/event/spk_event.hpp"

namespace spk
{
	class ControllerModule : public spk::Module<spk::ControllerEvent>
	{
	private:
		spk::Controller _controller;

		void _treatEvent(spk::ControllerEvent p_event) override;
		spk::ControllerEvent _convertEventToEventType(spk::Event p_event) override;

		void _handleJoystickMotion(const spk::ControllerEvent& p_event);
		void _handleTriggerMotion(const spk::ControllerEvent& p_event);
		void _handleDirectionalCrossMotion(const spk::ControllerEvent& p_event);
		void _handlePress(const spk::ControllerEvent& p_event);
		void _handleRelease(const spk::ControllerEvent& p_event);
		void _handleJoystickReset(const spk::ControllerEvent& p_event);
		void _handleTriggerReset(const spk::ControllerEvent& p_event);
		void _handleDirectionalCrossReset();

	public:
		ControllerModule() = default;

		const spk::Controller& controller() const;
	};
}
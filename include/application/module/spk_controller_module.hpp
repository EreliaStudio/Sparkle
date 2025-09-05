#pragma once

#include "application/module/spk_module.hpp"
#include "structure/system/event/spk_event.hpp"

namespace spk
{
	class ControllerModule : public spk::Module<spk::ControllerEvent>
	{
	private:
		spk::Controller _controller;

		void _treatEvent(spk::ControllerEvent &&p_event) override;
		spk::ControllerEvent _convertEventToEventType(spk::Event &&p_event) override;

		void _handleJoystickMotion(const spk::ControllerEvent &e);
		void _handleTriggerMotion(const spk::ControllerEvent &e);
		void _handleDirectionalCrossMotion(const spk::ControllerEvent &e);
		void _handlePress(const spk::ControllerEvent &e);
		void _handleRelease(const spk::ControllerEvent &e);
		void _handleJoystickReset(const spk::ControllerEvent &e);
		void _handleTriggerReset(const spk::ControllerEvent &e);
		void _handleDirectionalCrossReset(const spk::ControllerEvent &e);

		static bool _inDeadzone2D(const spk::Vector2Int &v, int dz) noexcept;
		static bool _nearZero(int v, int dz) noexcept;

	public:
		ControllerModule();

		const spk::Controller &controller() const;
	};
}

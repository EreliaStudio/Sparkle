#pragma once

#include "application/module/spk_module.hpp"

#include "structure/system/event/spk_event.hpp"

namespace spk
{
	class UpdateModule : public spk::Module<spk::UpdateEvent>
	{
	private:
		spk::SafePointer<spk::Widget> _widget;

		long long _lastNanoseconds = 0;
		long long _lastMilliseconds = 0;
		double _lastSeconds = 0.0f;

		spk::SafePointer<const spk::Mouse> _mouse = nullptr;
		spk::SafePointer<const spk::Keyboard> _keyboard = nullptr;
		spk::SafePointer<const spk::Controller> _controller = nullptr;

		void _treatEvent(spk::UpdateEvent&& p_event) override;
		spk::UpdateEvent _convertEventToEventType(spk::Event&& p_event) override;

	public:
		UpdateModule(spk::SafePointer<spk::Widget> p_widget);

		void bind(const spk::SafePointer<const Keyboard>& p_keyboard);
		void bind(const spk::SafePointer<const Mouse>& p_mouse);
		void bind(const spk::SafePointer<const Controller>& p_controller);
	};
}
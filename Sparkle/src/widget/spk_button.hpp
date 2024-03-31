#pragma once

#include "widget/spk_widget.hpp"
#include "design_pattern/spk_state_machine.hpp"

#include "widget/components/spk_nine_sliced.hpp"
#include "widget/components/spk_text_label.hpp"

namespace spk::widget
{
	class Button : public IWidget
	{
	public:
		using Callback = std::function<void()>;

		enum class State
		{
			Pressed = 0,
			Released = 1
		};

	private:
		StateMachine<State> _stateMachine;

		components::NineSlicedBox _boxes[2];
		components::TextLabel _labels[2];

		Callback _onClickCallback;

		Vector2 layout(const BoxConstraints&) override;
		void _onGeometryChange() override;
		void _onRender() override;
		void _onUpdate() override;

	public:
		Button(IWidget* p_parent);

		void setOnClickCallback(const Callback p_onClickCallback);

		widget::components::NineSlicedBox& box(const State& p_state);
		widget::components::TextLabel& label(const State& p_state);
	}; 
}
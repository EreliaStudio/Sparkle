#pragma once

#include "widget/spk_widget.hpp"
#include "design_pattern/spk_state_machine.hpp"

#include "widget/components/spk_nine_sliced.hpp"
#include "widget/components/spk_text_label.hpp"

namespace spk
{
	class Button : public Widget
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

		WidgetComponent::NineSlicedBox _boxes[2];
		WidgetComponent::TextLabel _labels[2];

		HorizontalAlignment _horizontalAlignment;
		VerticalAlignment _verticalAlignment;

		Callback _onClickCallback;

		void _updateBoxesGeometry();
		void _updateLabelsGeometry();
		void _onGeometryChange() override;
		void _onRender() override;
		void _onUpdate() override;

	public:
		Button(Widget* p_parent);

		void setOnClickCallback(const Callback p_onClickCallback);

		WidgetComponent::NineSlicedBox& box(const State& p_state);
		WidgetComponent::TextLabel& label(const State& p_state);

		void setLabelAlignment(const HorizontalAlignment& p_horizontalAlignment, const VerticalAlignment& p_verticalAlignment);
	}; 
}
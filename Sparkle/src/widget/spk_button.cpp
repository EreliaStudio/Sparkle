#include "widget/spk_button.hpp"

#include "application/spk_application.hpp"

namespace spk::widget
{
	Vector2 Button::layout(const BoxConstraints& p_constraints)
	{
		for (size_t i = 0; i < 2; i++)
		{
			_boxes[i].setGeometry(anchor(), size());
			_boxes[i].setLayer(layer());

			_labels[i].setLayer(layer() + 0.01f);
			_labels[i].setAnchor(anchor() + size() / 2); 
		}

		return (p_constraints.max);
	}

	void Button::_onGeometryChange()
	{
		
	}

	void Button::_onRender()
	{
		_boxes[static_cast<int>(_stateMachine.state())].render();
		_labels[static_cast<int>(_stateMachine.state())].render();
	}

	void Button::_onUpdate()
	{
		_stateMachine.update();
	}

	Button::Button(IWidget* p_parent) :
		IWidget(p_parent),
		_onClickCallback(nullptr),
		_stateMachine()
	{
		_stateMachine.addState(State::Released, StateMachine<State>::Action(
			[&](){},
			[&](){
				const auto& mouse = Application::activeApplication()->mouse();

				if (mouse.getButton(Mouse::Left) == InputState::Down &&
					hitTest(mouse.position()) == true)
				{
					_stateMachine.enterState(State::Pressed);
				}
			},
			[&](){}
			));
		_stateMachine.addState(State::Pressed, StateMachine<State>::Action(
			[&](){
				if (_onClickCallback != nullptr)
					_onClickCallback();
				},
			[&](){
				const auto& mouse = Application::activeApplication()->mouse();

				if (mouse.getButton(Mouse::Left) == InputState::Released ||
					hitTest(mouse.position()) == false)
				{
					_stateMachine.enterState(State::Released);
				}
			},
			[&](){}
			));

		_stateMachine.enterState(State::Released);
	}

	void Button::setOnClickCallback(const Callback p_onClickCallback)
	{
		_onClickCallback = p_onClickCallback;
	}

	widget::components::NineSlicedBox& Button::box(const State& p_state)
	{
		return (_boxes[static_cast<int>(p_state)]);
	}

	widget::components::TextLabel& Button::label(const State& p_state)
	{
		return (_labels[static_cast<int>(p_state)]);
	}
}
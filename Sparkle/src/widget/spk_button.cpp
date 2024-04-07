#include "widget/spk_button.hpp"

#include "application/spk_application.hpp"

namespace spk
{
	void Button::_onGeometryChange()
	{
		for (size_t i = 0; i < 2; i++)
		{
			_boxes[i].setGeometry(anchor(), size());
			_boxes[i].setLayer(layer());
			
			_labels[i].setGeometry(anchor() + _boxes[i].cornerSize(), size() - _boxes[i].cornerSize() * 2);
			_labels[i].setLayer(layer() + 0.01f);
		}
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

	Button::Button(Widget* p_parent) :
		Widget(p_parent),
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

	WidgetComponent::NineSlicedBox& Button::box(const State& p_state)
	{
		return (_boxes[static_cast<int>(p_state)]);
	}

	WidgetComponent::TextLabel& Button::label(const State& p_state)
	{
		return (_labels[static_cast<int>(p_state)]);
	}

	void Button::setText(const std::string& p_buttonText)
	{
		for (size_t i = 0; i < 2; i++)
		{
			_labels[i].setText(p_buttonText);
		}
	}
}
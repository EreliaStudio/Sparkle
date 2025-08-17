#include "structure/engine/spk_top_down_view_controller.hpp"

#include "structure/engine/spk_entity.hpp"
#include "structure/math/spk_quaternion.hpp"

namespace spk
{
	void TopDownViewController::_applyConfiguration()
	{
		dynamic_cast<spk::KeyboardAction *>(_actions[0].get())->setDeviceValue(_config.keymap[L"Forward"], spk::InputState::Down);
		dynamic_cast<spk::KeyboardAction *>(_actions[1].get())->setDeviceValue(_config.keymap[L"Left"], spk::InputState::Down);
		dynamic_cast<spk::KeyboardAction *>(_actions[2].get())->setDeviceValue(_config.keymap[L"Backward"], spk::InputState::Down);
		dynamic_cast<spk::KeyboardAction *>(_actions[3].get())->setDeviceValue(_config.keymap[L"Right"], spk::InputState::Down);
		dynamic_cast<spk::KeyboardAction *>(_actions[4].get())->setDeviceValue(_config.keymap[L"RotateLeft"], spk::InputState::Down);
		dynamic_cast<spk::KeyboardAction *>(_actions[5].get())->setDeviceValue(_config.keymap[L"RotateRight"], spk::InputState::Down);
	}

	TopDownViewController::TopDownViewController(const std::wstring &p_name) :
		spk::Component(p_name)
	{
		_actions.push_back(std::make_unique<spk::KeyboardAction>(_config.keymap[L"Forward"],
																 spk::InputState::Down,
																 10,
																 [&](const spk::SafePointer<const spk::Keyboard> &p_keyboard)
																 {
																	 spk::Vector3 forward = owner()->transform().forward() * spk::Vector3(1, 0, 1);
																	 
																	 if (forward == spk::Vector3())
																	 {
																		forward = owner()->transform().up() * spk::Vector3(1, 0, 1);
																	 }

																	 _motionRequested += forward;
																 }));

		_actions.push_back(std::make_unique<spk::KeyboardAction>(_config.keymap[L"Left"],
																 spk::InputState::Down,
																 10,
																 [&](const spk::SafePointer<const spk::Keyboard> &p_keyboard)
																 {
																	 spk::Vector3 right = owner()->transform().right();
																	 right.y = 0;
																	 if (right != spk::Vector3())
																	 {
																		 right = right.normalize();
																	 }
																	 _motionRequested -= right;
																 }));

		_actions.push_back(std::make_unique<spk::KeyboardAction>(_config.keymap[L"Backward"],
																 spk::InputState::Down,
																 10,
																 [&](const spk::SafePointer<const spk::Keyboard> &p_keyboard)
																 {
																	 spk::Vector3 forward = owner()->transform().forward() * spk::Vector3(1, 0, 1);
																	 
																	 if (forward == spk::Vector3())
																	 {
																		forward = owner()->transform().up() * spk::Vector3(1, 0, 1);
																	 }

																	 _motionRequested -= forward;
																 }));

		_actions.push_back(std::make_unique<spk::KeyboardAction>(_config.keymap[L"Right"],
																 spk::InputState::Down,
																 10,
																 [&](const spk::SafePointer<const spk::Keyboard> &p_keyboard)
																 {
																	 spk::Vector3 right = owner()->transform().right();
																	 right.y = 0;
																	 if (right != spk::Vector3())
																	 {
																		 right = right.normalize();
																	 }
																	 _motionRequested += right;
																 }));

		_actions.push_back(std::make_unique<spk::KeyboardAction>(_config.keymap[L"RotateLeft"],
																 spk::InputState::Down,
																 10,
																 [&](const spk::SafePointer<const spk::Keyboard> &p_keyboard)
																 { _rotationRequested += 1.0f; }));

		_actions.push_back(std::make_unique<spk::KeyboardAction>(_config.keymap[L"RotateRight"],
																 spk::InputState::Down,
																 10,
																 [&](const spk::SafePointer<const spk::Keyboard> &p_keyboard)
																 { _rotationRequested -= 1.0f; }));

		_focusPoint = owner()->transform().position() + owner()->transform().forward();
	}

	void TopDownViewController::setConfiguration(const Configuration &p_configuration)
	{
		_config = p_configuration;
		_applyConfiguration();
	}

	const TopDownViewController::Configuration &TopDownViewController::configuration() const
	{
		return (_config);
	}

	void TopDownViewController::setFocus(const spk::Vector3 &p_focus)
	{
		_focusPoint = p_focus;
		owner()->transform().lookAt(_focusPoint);
	}

	const spk::Vector3 &TopDownViewController::focus() const
	{
		return (_focusPoint);
	}

	void TopDownViewController::onUpdateEvent(spk::UpdateEvent &p_event)
	{
		if (p_event.keyboard == nullptr)
		{
			return;
		}

		_motionRequested = spk::Vector3(0, 0, 0);
		_rotationRequested = 0.0f;

		for (auto &action : _actions)
		{
			if (action->isInitialized() == false)
			{
				action->initialize(p_event);
			}

			action->update();
		}

		bool isMotionRequested = _motionRequested != spk::Vector3();
		bool isRotationRequested = _rotationRequested != 0.0f;

		if (isMotionRequested == true || isRotationRequested == true)
		{
			if (_motionRequested != spk::Vector3())
			{
				spk::Vector3 delta = _motionRequested.normalize() * (float)p_event.deltaTime.seconds * _config.moveSpeed;
				owner()->transform().move(delta);
				_focusPoint += delta;
			}

			if (_rotationRequested != 0.0f)
			{
				float angle = _rotationRequested * (float)p_event.deltaTime.seconds * _config.rotateSpeed;
				spk::Vector3 offset = owner()->transform().position() - _focusPoint;
				spk::Quaternion rot = spk::Quaternion::fromAxisAngle({0, 1, 0}, angle);
				offset = rot * offset;
				owner()->transform().place(_focusPoint + offset);
				owner()->transform().lookAt(_focusPoint);
			}

			p_event.requestPaint();
		}
	}
}

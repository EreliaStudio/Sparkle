#include "structure/engine/spk_free_view_controller.hpp"

#include "structure/engine/spk_entity.hpp"

namespace spk
{
	void FreeViewController::_applyConfiguration()
	{
		dynamic_cast<spk::KeyboardAction *>(_actions[0].get())->setDeviceValue(_config.keymap[L"Forward"], spk::InputState::Down);
		dynamic_cast<spk::KeyboardAction *>(_actions[1].get())->setDeviceValue(_config.keymap[L"Left"], spk::InputState::Down);
		dynamic_cast<spk::KeyboardAction *>(_actions[2].get())->setDeviceValue(_config.keymap[L"Backward"], spk::InputState::Down);
		dynamic_cast<spk::KeyboardAction *>(_actions[3].get())->setDeviceValue(_config.keymap[L"Right"], spk::InputState::Down);
		dynamic_cast<spk::KeyboardAction *>(_actions[4].get())->setDeviceValue(_config.keymap[L"Up"], spk::InputState::Down);
		dynamic_cast<spk::KeyboardAction *>(_actions[5].get())->setDeviceValue(_config.keymap[L"Down"], spk::InputState::Down);

		dynamic_cast<spk::MouseButtonAction *>(_actions[6].get())->setDeviceValue(_config.lookButton, spk::InputState::Down);
		dynamic_cast<spk::MouseButtonAction *>(_actions[7].get())->setDeviceValue(_config.lookButton, spk::InputState::Up);
	}

	FreeViewController::FreeViewController(const std::wstring &p_name) :
		spk::Component(p_name)
	{
		_actions.push_back(
			std::make_unique<spk::KeyboardAction>(
				_config.keymap[L"Forward"],
				spk::InputState::Down,
				10,
				[&](const spk::SafePointer<const spk::Keyboard> &p_keyboard) { _motionRequested += owner()->transform().forward(); }));

		_actions.push_back(
			std::make_unique<spk::KeyboardAction>(
				_config.keymap[L"Left"],
				spk::InputState::Down,
				10,
				[&](const spk::SafePointer<const spk::Keyboard> &p_keyboard) { _motionRequested -= owner()->transform().right(); }));

		_actions.push_back(
			std::make_unique<spk::KeyboardAction>(
				_config.keymap[L"Backward"],
				spk::InputState::Down,
				10,
				[&](const spk::SafePointer<const spk::Keyboard> &p_keyboard) { _motionRequested -= owner()->transform().forward(); }));

		_actions.push_back(
			std::make_unique<spk::KeyboardAction>(
				_config.keymap[L"Right"],
				spk::InputState::Down,
				10,
				[&](const spk::SafePointer<const spk::Keyboard> &p_keyboard) { _motionRequested += owner()->transform().right(); }));

		_actions.push_back(
			std::make_unique<spk::KeyboardAction>(
				_config.keymap[L"Up"],
				spk::InputState::Down,
				10,
				[&](const spk::SafePointer<const spk::Keyboard> &p_keyboard)
				{
					if (_config.allowFly == true)
					{
						_motionRequested += spk::Vector3{0, 1, 0};
					}
				}));

		_actions.push_back(
			std::make_unique<spk::KeyboardAction>(
				_config.keymap[L"Down"],
				spk::InputState::Down,
				10,
				[&](const spk::SafePointer<const spk::Keyboard> &p_keyboard)
				{
					if (_config.allowFly == true)
					{
						_motionRequested -= spk::Vector3{0, 1, 0};
					}
				}));

		_actions.push_back(
			std::make_unique<spk::MouseButtonAction>(
				spk::Mouse::Button::Left,
				spk::InputState::Down,
				-1,
				[&](const spk::SafePointer<const spk::Mouse> &p_mouse)
				{
					_isMovingCamera = true;
					_lastMousePosition = p_mouse->position();
				}));
		_actions.push_back(
			std::make_unique<spk::MouseButtonAction>(
				spk::Mouse::Button::Left,
				spk::InputState::Up,
				-1,
				[&](const spk::SafePointer<const spk::Mouse> &p_mouse) { _isMovingCamera = false; }));

		_actions.push_back(
			std::make_unique<spk::MouseMotionAction>(
				spk::MouseMotionAction::Mode::Absolute,
				[&](const spk::Vector2Int &p_mousePosition)
				{
					if (_isMovingCamera == false)
					{
						return;
					}

					spk::Vector2Int deltaPosition = p_mousePosition - _lastMousePosition;

					if (deltaPosition != spk::Vector2Int(0, 0))
					{
						_rotationRequested.x = static_cast<float>(deltaPosition.x) * _config.mouseSensitivity;
						_rotationRequested.y = static_cast<float>(deltaPosition.y) * _config.mouseSensitivity;
					}
				}));
	}

	void FreeViewController::setConfiguration(const Configuration &p_configuration)
	{
		_config = p_configuration;
		_applyConfiguration();
	}

	const FreeViewController::Configuration &FreeViewController::configuration() const
	{
		return (_config);
	}

	void FreeViewController::onUpdateEvent(spk::UpdateEvent &p_event)
	{
		if (p_event.keyboard == nullptr || p_event.mouse == nullptr)
		{
			return;
		}

		_motionRequested = spk::Vector3(0, 0, 0);
		_rotationRequested = spk::Vector2(0, 0);

		for (auto &action : _actions)
		{
			if (action->isInitialized() == false)
			{
				action->initialize(p_event);
			}

			action->update();
		}

		bool isMotionRequested = _motionRequested != spk::Vector3();
		bool isRotationRequested = _rotationRequested != spk::Vector2();

		if (isMotionRequested == true || isRotationRequested == true)
		{
			if (_motionRequested != spk::Vector3())
			{
				auto delta = _motionRequested.normalize() * (float)p_event.deltaTime.seconds * _config.moveSpeed;
				owner()->transform().move(delta);
			}

			if (_rotationRequested != spk::Vector2())
			{
				owner()->transform().rotateAroundAxis({0, 1, 0}, _rotationRequested.x);
				owner()->transform().rotateAroundAxis(owner()->transform().right(), _rotationRequested.y);
				p_event.mouse->place(_lastMousePosition);
			}

			p_event.requestPaint();
		}
	}
}
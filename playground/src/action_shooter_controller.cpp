#include "action_shooter_controller.hpp"
#include <sparkle.hpp>
#include "event.hpp"

namespace taag
{
	void ActionShooterController::_applyConfiguration()
	{
		dynamic_cast<spk::KeyboardAction *>(_actions[0].get())->setDeviceValue(_config.keymap[L"Forward"], spk::InputState::Down);
		dynamic_cast<spk::KeyboardAction *>(_actions[1].get())->setDeviceValue(_config.keymap[L"Left"], spk::InputState::Down);
		dynamic_cast<spk::KeyboardAction *>(_actions[2].get())->setDeviceValue(_config.keymap[L"Backward"], spk::InputState::Down);
		dynamic_cast<spk::KeyboardAction *>(_actions[3].get())->setDeviceValue(_config.keymap[L"Right"], spk::InputState::Down);
	}

	ActionShooterController::ActionShooterController(const std::wstring &p_name) :
		spk::Component(p_name)
	{
		_actions.push_back(
			std::make_unique<spk::KeyboardAction>(
				_config.keymap[L"Forward"],
				spk::InputState::Down,
				10,
				[&](const spk::SafePointer<const spk::Keyboard> &p_keyboard) { _motionRequested += spk::Vector3(0, 1, 0); }));

		_actions.push_back(
			std::make_unique<spk::KeyboardAction>(
				_config.keymap[L"Left"],
				spk::InputState::Down,
				10,
				[&](const spk::SafePointer<const spk::Keyboard> &p_keyboard) { _motionRequested -= spk::Vector3(1, 0, 0); }));

		_actions.push_back(
			std::make_unique<spk::KeyboardAction>(
				_config.keymap[L"Backward"],
				spk::InputState::Down,
				10,
				[&](const spk::SafePointer<const spk::Keyboard> &p_keyboard) { _motionRequested -= spk::Vector3(0, 1, 0); }));

		_actions.push_back(
			std::make_unique<spk::KeyboardAction>(
				_config.keymap[L"Right"],
				spk::InputState::Down,
				10,
				[&](const spk::SafePointer<const spk::Keyboard> &p_keyboard) { _motionRequested += spk::Vector3(1, 0, 0); }));
	}

	void ActionShooterController::setConfiguration(const Configuration &p_configuration)
	{
		_config = p_configuration;
		_applyConfiguration();
	}

	void ActionShooterController::awake()
	{
		_player = owner();
		_camera = owner()->getChild(owner()->name() + L"/CameraHolder");
	}

	void ActionShooterController::onUpdateEvent(spk::UpdateEvent &p_event)
	{
		if ((p_event.keyboard == nullptr) == true)
		{
			return;
		}

		_motionRequested = spk::Vector3(0, 0, 0);

		for (auto &action : _actions)
		{
			if ((action->isInitialized() == false) == true)
			{
				action->initialize(p_event);
			}

			action->update();
		}

		bool isMotionRequested = (_motionRequested != spk::Vector3());
		if (isMotionRequested == true)
		{
			spk::Vector3 delta = _motionRequested.normalize() * (float)p_event.deltaTime.seconds * _config.moveSpeed;
			_player->transform().move(delta);
			p_event.requestPaint();
			EventDispatcher::emit(Event::PlayerMotion);
		}
	}

	void ActionShooterController::onMouseEvent(spk::MouseEvent &p_event)
	{
		if (p_event.type == spk::MouseEvent::Type::Motion)
		{
			spk::SafePointer<spk::Window> window = p_event.window;

			const spk::Geometry2D &windowGeometry = window->geometry();
			spk::Vector2Int windowCenter(static_cast<int>(windowGeometry.width) / 2, static_cast<int>(windowGeometry.height) / 2);

			if (p_event.mouse->position() != windowCenter)
			{
				spk::Vector2Int delta = p_event.mouse->position() - windowCenter;
				float mouseAngleRad = std::atan2(static_cast<float>(delta.y), static_cast<float>(delta.x));

				float deltaAngleRad = std::atan2(std::sin(mouseAngleRad - _lastMouseAngleRad), std::cos(mouseAngleRad - _lastMouseAngleRad));
				float deltaAngleDeg = spk::radianToDegree(deltaAngleRad);

				_player->transform().rotateAroundAxis(spk::Vector3(0, 0, 1), deltaAngleDeg);
				_camera->transform().rotateAroundAxis(spk::Vector3(0, 0, 1), -deltaAngleDeg);

				_lastMouseAngleRad = mouseAngleRad;

				p_event.requestPaint();
			}
		}
	}

}
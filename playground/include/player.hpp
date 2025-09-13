#pragma once

#include <sparkle.hpp>

#include "action_shooter_controller.hpp"
#include "camera_holder.hpp"
#include "shape.hpp"
#include "event.hpp"

namespace taag
{
	class Player : public Shape
	{
	private:
		EventDispatcher::Instanciator _eventDispatcherInstanciator;
		CameraHolder _cameraHolder;
		spk::SafePointer<ActionShooterController> _controller;

	public:
		Player(const std::wstring &p_name, spk::SafePointer<spk::Entity> p_parent) :
			Shape(p_name, p_parent),
			_cameraHolder(CameraHolder(p_name + L"/CameraHolder", this)),
			_controller(addComponent<ActionShooterController>(p_name + L"/ActionShooterController"))
		{
			_cameraHolder.setAsMainCamera();
			transform().addOnEditionCallback([&](){EventDispatcher::emit(Event::PlayerMotion);}).relinquish();
		}

		spk::SafePointer<CameraHolder> camera()
		{
			return (&_cameraHolder);
		}

		spk::SafePointer<const CameraHolder> camera() const
		{
			return (&_cameraHolder);
		}
	};
}
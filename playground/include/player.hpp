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
		Player(const std::wstring &p_name, spk::SafePointer<spk::Entity> p_parent);

		spk::SafePointer<CameraHolder> camera();
		spk::SafePointer<const CameraHolder> camera() const;
	};
}
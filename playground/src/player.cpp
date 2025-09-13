#include "player.hpp"

namespace taag
{
	Player::Player(const std::wstring &p_name, spk::SafePointer<spk::Entity> p_parent) :
		Shape(p_name, p_parent),
		_cameraHolder(CameraHolder(p_name + L"/CameraHolder", this)),
		_controller(addComponent<ActionShooterController>(p_name + L"/ActionShooterController"))
	{
		_cameraHolder.setAsMainCamera();
	}

	spk::SafePointer<CameraHolder> Player::camera()
	{
		return (&_cameraHolder);
	}

	spk::SafePointer<const CameraHolder> Player::camera() const
	{
		return (&_cameraHolder);
	}
}
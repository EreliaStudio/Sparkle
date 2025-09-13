#include "camera_holder.hpp"

namespace taag
{
	CameraHolder::CameraHolder(const std::wstring &p_name, spk::SafePointer<spk::Entity> p_parent) :
		spk::Entity(p_name, p_parent),
		_cameraComponent(addComponent<spk::CameraComponent>(p_name + L"/CameraComponent"))
	{
		transform().place({0.0f, 0.0f, 20.0f});
		transform().lookAtLocal({0, 0, 0});
	}
	
	const spk::Camera &CameraHolder::camera() const
	{
		return (_cameraComponent->camera());
	}
	
	void CameraHolder::setOrthographic(spk::Vector2 p_viewSize)
	{
		_cameraComponent->setOrthographic(p_viewSize.x, p_viewSize.y);
		EventDispatcher::emit(Event::RefreshView);
	}
	
	void CameraHolder::setAsMainCamera() const
	{
		_mainCamera = this;
	}
	
	spk::SafePointer<const CameraHolder> CameraHolder::mainCamera()
	{
		return (_mainCamera);
	}
}
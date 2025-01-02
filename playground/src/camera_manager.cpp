#include "camera_manager.hpp"

#include "event.hpp"

CameraManager::CameraManager(const std::wstring& p_name) :
	spk::Component(p_name),
	_cameraUBO(BufferObjectCollection::instance()->UBO("camera"))
{
	setOrthographic(-20, 20, -20, 20);
}

void CameraManager::start()
{
	_onEditionContract = owner()->transform().addOnEditionCallback([&](){
		_cameraUBO["view"] = owner()->transform().inverseModel();
	});
	
	_onEditionContract.trigger();
}

void CameraManager::setOrthographic(float p_left, float p_right, float p_bottom, float p_top, float p_nearPlane, float p_farPlane)
{
	_camera.setOrthographic(p_left, p_right, p_bottom, p_top, p_nearPlane, p_farPlane);
	_cameraUBO["projection"] = _camera.projectionMatrix();
}

const spk::Camera& CameraManager::camera() const
{
	return (_camera);
}

void CameraManager::onPaintEvent(spk::PaintEvent& p_event)
{
	if (p_event.type == spk::PaintEvent::Type::Resize)
	{
		spk::Vector2Int cellSize = 32;
		spk::Vector2 cellCount = p_event.geometry.size / cellSize;

		setOrthographic(cellCount.x / -2, cellCount.x / 2, cellCount.y / -2, cellCount.y / 2);
		EventCenter::instance()->notifyEvent(Event::UpdateChunkVisibility);
	}
}
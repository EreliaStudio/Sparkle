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
	_camera.setOrthographic(p_left, p_right, p_top, p_bottom, p_nearPlane, p_farPlane);
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
		_geometry = p_event.geometry;

		spk::Vector2 cellCount = _geometry.size / _cellSize;

		setOrthographic(cellCount.x / -2, cellCount.x / 2, cellCount.y / -2, cellCount.y / 2);
		EventCenter::instance()->notifyEvent(Event::UpdateChunkVisibility);
	}
}

void CameraManager::onMouseEvent(spk::MouseEvent& p_event)
{
	switch (p_event.type)
	{
		case spk::MouseEvent::Type::Motion:
		{
			spk::Matrix4x4 inverseMatrix = _camera.inverseProjectionMatrix();

			spk::Vector2 mouseRatio = (static_cast<spk::Vector2>(p_event.mouse->position) / static_cast<spk::Vector2>(_geometry.size)) * 2 - 1;

			spk::cout << "Mouse at tile [" << owner()->position() + (inverseMatrix * spk::Vector3(mouseRatio, 0)) << "]" << std::endl; 
		}
	}
}
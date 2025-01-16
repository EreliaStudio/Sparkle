#pragma once

#include "buffer_object_collection.hpp"

#include "structure/engine/spk_entity.hpp"
#include "structure/engine/spk_component.hpp"
#include "structure/engine/spk_camera.hpp"

class CameraManager : public spk::Component
{
private:
	BufferObjectCollection::Instanciator _bindingPointInstanciator;
	spk::OpenGL::UniformBufferObject& _cameraUBO;

	static inline const spk::Vector2Int _cellSize = 32;
	spk::Geometry2D _geometry;
	spk::Camera _camera;

	spk::Entity::Contract _onEditionContract;

public:
	CameraManager(const std::wstring& p_name);

	void start() override;

	void setOrthographic(float p_left, float p_right, float p_bottom, float p_top, float p_nearPlane= 0.1f, float p_farPlane = 1000.0f);

	const spk::Camera& camera() const;

	void onPaintEvent(spk::PaintEvent& p_event) override;
	void onMouseEvent(spk::MouseEvent& p_event) override;
};
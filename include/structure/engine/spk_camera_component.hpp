#pragma once

#include "structure/engine/spk_component.hpp"
#include "structure/engine/spk_camera.hpp"

namespace spk
{
	class CameraComponent : public spk::Component
	{
	private:
		spk::Camera _camera;
		spk::ContractProvider::Contract _transformContract;

		void _updateUBO();

	public:
		CameraComponent(const std::wstring &p_name);

		void setPerspective(float p_fov, float p_aspectRatio, float p_near = 0.1f, float p_far = 1000.0f);

		void start() override;
		void onGeometryChange(const spk::Geometry2D& p_geometry) override;

		spk::Camera &camera();
	};
}
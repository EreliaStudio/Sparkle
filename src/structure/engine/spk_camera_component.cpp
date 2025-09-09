#include "structure/engine/spk_camera_component.hpp"

#include "structure/engine/spk_entity.hpp"

#include "structure/graphics/lumina/spk_shader.hpp"

namespace spk
{
	void CameraComponent::_updateUBO()
	{
		auto &cameraUBO = spk::Lumina::Shader::Constants::ubo(L"CameraUBO");
		const spk::Transform &transform = owner()->transform();
		cameraUBO->layout()[L"viewMatrix"] = transform.inverseModel();
		cameraUBO->layout()[L"projectionMatrix"] = _camera.projectionMatrix();
		cameraUBO->validate();
	}

	CameraComponent::CameraComponent(const std::wstring &p_name) :
		spk::Component(p_name)
	{
		_policy.mode = Policy::Mode::Perspective;
		_policy.nearPlane = 0.1f;
		_policy.farPlane = 1000.0f;
		_policy.data.perspective.fovDegrees = 60.0f;
		_policy.data.perspective.fallbackAspect = 1.0f;
	}

	void CameraComponent::setPerspective(float p_fov, float p_aspectRatio, float p_near, float p_far)
	{
		// One perspective mode: store FOV and near/far; keep provided aspect as fallback until first geometry
		_policy.mode = Policy::Mode::Perspective;
		_policy.nearPlane = p_near;
		_policy.farPlane = p_far;
		_policy.data.perspective.fovDegrees = p_fov;
		_policy.data.perspective.fallbackAspect = p_aspectRatio;
		_camera.setPerspective(p_fov, p_aspectRatio, p_near, p_far);
		_updateUBO();
	}

	void CameraComponent::setOrthographic(float p_cellPixelWidth, float p_cellPixelHeight, float p_near, float p_far)
	{
		_policy.mode = Policy::Mode::Orthographic;
		_policy.nearPlane = p_near;
		_policy.farPlane = p_far;
		_policy.data.orthographic.pixelsPerUnitX = p_cellPixelWidth;
		_policy.data.orthographic.pixelsPerUnitY = p_cellPixelHeight;
		if (_hasLastGeometry == true)
		{
			_applyProjectionForGeometry(_lastGeometry);
		}
		else
		{
			_updateUBO();
		}
	}

	void CameraComponent::start()
	{
		_transformContract = owner()->transform().addOnEditionCallback([this]() { _updateUBO(); });
		_updateUBO();
	}

	void CameraComponent::onGeometryChange(const spk::Geometry2D &p_geometry)
	{
		_lastGeometry = p_geometry;
		_hasLastGeometry = true;
		_applyProjectionForGeometry(p_geometry);
	}

	const spk::Camera &CameraComponent::camera() const
	{
		return _camera;
	}

	void CameraComponent::_applyProjectionForGeometry(const spk::Geometry2D &p_geometry)
	{
		const float width = static_cast<float>(p_geometry.size.x);
		const float height = static_cast<float>(p_geometry.size.y);

		switch (_policy.mode)
		{
		case Policy::Mode::Perspective:
		{
			float aspect = (height != 0.0f) ? (width / height) : 1.0f;
			if (height == 0.0f)
			{
				aspect = _policy.data.perspective.fallbackAspect;
			}
			const float vFov = _policy.data.perspective.fovDegrees;
			_camera.setPerspective(vFov, aspect, _policy.nearPlane, _policy.farPlane);
			break;
		}
		case Policy::Mode::Orthographic:
		{
			const float ppuX = (_policy.data.orthographic.pixelsPerUnitX != 0.0f) ? _policy.data.orthographic.pixelsPerUnitX : 1.0f;
			const float ppuY = (_policy.data.orthographic.pixelsPerUnitY != 0.0f) ? _policy.data.orthographic.pixelsPerUnitY : 1.0f;
			const float worldWidth = width / ppuX;
			const float worldHeight = height / ppuY;
			const float halfW = worldWidth * 0.5f;
			const float halfH = worldHeight * 0.5f;
			_camera.setOrthographic(-halfW, halfW, -halfH, halfH, _policy.nearPlane, _policy.farPlane);
			break;
		}
		default:
		{
			break;
		}
		}
		
		_updateUBO();
	}
}

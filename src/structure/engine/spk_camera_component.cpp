#include "structure/engine/spk_camera_component.hpp"

#include "structure/engine/spk_entity.hpp"

#include "structure/graphics/lumina/spk_shader.hpp"

namespace spk
{
	void CameraComponent::_updateUBO()
	{
		auto &cameraUBO = spk::Lumina::Shader::Constants::ubo(L"CameraUBO");
		const spk::Transform &transform = owner()->transform();
		cameraUBO.layout()[L"viewMatrix"] = transform.inverseModel();
		cameraUBO.layout()[L"projectionMatrix"] = _camera.projectionMatrix();
		cameraUBO.validate();
	}

	CameraComponent::CameraComponent(const std::wstring &p_name) :
		spk::Component(p_name)
	{
	}

	void CameraComponent::setPerspective(float p_fov, float p_aspectRatio, float p_near, float p_far)
	{
		_camera.setPerspective(p_fov, p_aspectRatio, p_near, p_far);
		_updateUBO();
	}

	void CameraComponent::start()
	{
		_transformContract = owner()->transform().addOnEditionCallback([this]() { _updateUBO(); });
		_updateUBO();
	}

	void CameraComponent::onGeometryChange(const spk::Geometry2D& p_geometry)
	{
		setPerspective(60.0f, static_cast<float>(p_geometry.size.x) / static_cast<float>(p_geometry.size.y));
	}

	spk::Camera &CameraComponent::camera()
	{
		return _camera;
	}
}